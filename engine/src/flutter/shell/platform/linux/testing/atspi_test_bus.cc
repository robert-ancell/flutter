// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/testing/atspi_test_bus.h"

#include <sys/socket.h>

#include "gtest/gtest.h"

namespace flutter {
namespace testing {

namespace {

constexpr char kRegistryXml[] = R"xml(<node>
  <interface name="org.a11y.atspi.Registry">
    <method name="GetRegisteredEvents">
      <arg direction="out" type="a(ss)"/>
    </method>
  </interface>
</node>)xml";

void connection_ready_cb(GObject* object,
                         GAsyncResult* result,
                         gpointer user_data) {
  *static_cast<GDBusConnection**>(user_data) =
      g_dbus_connection_new_finish(result, nullptr);
}

}  // namespace

AtspiTestBus::AtspiTestBus() {
  int fds[2];
  EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds), 0);
  g_autoptr(GSocket) app_socket = g_socket_new_from_fd(fds[0], nullptr);
  g_autoptr(GSocket) peer_socket = g_socket_new_from_fd(fds[1], nullptr);
  g_autoptr(GSocketConnection) app_stream =
      g_socket_connection_factory_create_connection(app_socket);
  g_autoptr(GSocketConnection) peer_stream =
      g_socket_connection_factory_create_connection(peer_socket);

  g_autofree gchar* guid = g_dbus_generate_guid();
  g_dbus_connection_new(G_IO_STREAM(peer_stream), guid,
                        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER, nullptr,
                        nullptr, connection_ready_cb, &peer_);
  g_dbus_connection_new(G_IO_STREAM(app_stream), nullptr,
                        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, nullptr,
                        nullptr, connection_ready_cb, &app_connection_);
  EXPECT_TRUE(RunUntil(
      [this]() { return peer_ != nullptr && app_connection_ != nullptr; }));
  if (peer_ == nullptr) {
    return;
  }

  signal_subscription_ = g_dbus_connection_signal_subscribe(
      peer_, nullptr, nullptr, nullptr, nullptr, nullptr,
      G_DBUS_SIGNAL_FLAGS_NONE,
      [](GDBusConnection* connection, const gchar* sender, const gchar* path,
         const gchar* interface, const gchar* member, GVariant* parameters,
         gpointer user_data) {
        static_cast<AtspiTestBus*>(user_data)->signals_.push_back(
            {path, interface, member, g_variant_ref(parameters)});
      },
      this, nullptr);
}

AtspiTestBus::~AtspiTestBus() {
  ClearSignals();
  for (GVariant* reply : replies_) {
    g_variant_unref(reply);
  }
  if (peer_ != nullptr) {
    if (registry_id_ != 0) {
      g_dbus_connection_unregister_object(peer_, registry_id_);
    }
    g_dbus_connection_signal_unsubscribe(peer_, signal_subscription_);
    g_dbus_connection_close_sync(peer_, nullptr, nullptr);
  }
  g_clear_object(&peer_);
  g_clear_object(&app_connection_);
  g_clear_pointer(&registry_info_, g_dbus_node_info_unref);
}

void AtspiTestBus::ServeRegistry(const std::vector<std::string>& events) {
  registered_events_ = events;
  registry_info_ = g_dbus_node_info_new_for_xml(kRegistryXml, nullptr);
  static const GDBusInterfaceVTable kRegistryVTable = {
      .method_call =
          [](GDBusConnection* connection, const gchar* sender,
             const gchar* path, const gchar* interface, const gchar* method,
             GVariant* parameters, GDBusMethodInvocation* invocation,
             gpointer user_data) {
            auto* self = static_cast<AtspiTestBus*>(user_data);
            g_autoptr(GVariantBuilder) builder =
                g_variant_builder_new(G_VARIANT_TYPE("a(ss)"));
            for (const std::string& event : self->registered_events_) {
              g_variant_builder_add(builder, "(ss)", ":1.7", event.c_str());
            }
            g_dbus_method_invocation_return_value(
                invocation, g_variant_new("(a(ss))", builder));
            self->registry_called_ = true;
          },
  };
  registry_id_ = g_dbus_connection_register_object(
      peer_, "/org/a11y/atspi/registry", registry_info_->interfaces[0],
      &kRegistryVTable, this, nullptr, nullptr);
  EXPECT_NE(registry_id_, 0u);
}

GVariant* AtspiTestBus::Call(const std::string& path,
                             const std::string& interface,
                             const std::string& member,
                             GVariant* parameters,
                             GError** error) {
  struct Result {
    bool done = false;
    GVariant* reply = nullptr;
    GError* error = nullptr;
  } result;
  g_dbus_connection_call(
      peer_, nullptr, path.c_str(), interface.c_str(), member.c_str(),
      parameters, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
      [](GObject* object, GAsyncResult* async_result, gpointer user_data) {
        Result* result = static_cast<Result*>(user_data);
        result->reply = g_dbus_connection_call_finish(
            G_DBUS_CONNECTION(object), async_result, &result->error);
        result->done = true;
      },
      &result);
  EXPECT_TRUE(RunUntil([&result]() { return result.done; }));
  if (result.reply != nullptr) {
    replies_.push_back(result.reply);
  }
  if (error != nullptr) {
    *error = result.error;
  } else {
    g_clear_error(&result.error);
  }
  return result.reply;
}

GVariant* AtspiTestBus::GetProperty(const std::string& path,
                                    const std::string& interface,
                                    const std::string& name) {
  GVariant* reply =
      Call(path, "org.freedesktop.DBus.Properties", "Get",
           g_variant_new("(ss)", interface.c_str(), name.c_str()));
  if (reply == nullptr) {
    return nullptr;
  }
  GVariant* value;
  g_variant_get(reply, "(v)", &value);
  replies_.push_back(value);
  return value;
}

void AtspiTestBus::Sync() {
  EXPECT_NE(Call("/", "org.freedesktop.DBus.Peer", "Ping"), nullptr);
}

void AtspiTestBus::EmitSignal(const std::string& path,
                              const std::string& interface,
                              const std::string& member,
                              GVariant* parameters) {
  g_dbus_connection_emit_signal(peer_, nullptr, path.c_str(), interface.c_str(),
                                member.c_str(), parameters, nullptr);
}

void AtspiTestBus::ClearSignals() {
  for (Signal& signal : signals_) {
    g_variant_unref(signal.parameters);
  }
  signals_.clear();
}

std::string AtspiTestBus::GetChildPath(const std::string& path, int index) {
  GVariant* reply = Call(path, "org.a11y.atspi.Accessible", "GetChildAtIndex",
                         g_variant_new("(i)", index));
  if (reply == nullptr) {
    return "";
  }
  const gchar* child_path;
  g_variant_get(reply, "((&s&o))", nullptr, &child_path);
  return child_path;
}

std::vector<std::string> AtspiTestBus::Describe(
    const std::map<std::string, std::string>& labels) {
  std::vector<std::string> result;
  for (const Signal& signal : signals_) {
    auto label = labels.find(signal.path);
    std::string path = label != labels.end() ? label->second : signal.path;
    const gchar* detail = nullptr;
    gint32 detail1 = 0;
    g_variant_get(signal.parameters, "(&siiva{sv})", &detail, &detail1, nullptr,
                  nullptr, nullptr);
    std::string text = path + " " + signal.member;
    if (detail != nullptr && detail[0] != '\0') {
      text += std::string(":") + detail;
    }
    text += " " + std::to_string(detail1);
    result.push_back(text);
  }
  return result;
}

}  // namespace testing
}  // namespace flutter
