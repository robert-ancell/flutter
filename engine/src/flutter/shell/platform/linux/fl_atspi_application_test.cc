// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Included first as it collides with the X11 headers.
#include "flutter/shell/platform/linux/testing/linux_test.h"
#include "gtest/gtest.h"

#include <memory>
#include <string>

#include "flutter/shell/platform/linux/fl_atspi_application.h"
#include "flutter/shell/platform/linux/fl_view_accessible.h"
#include "flutter/shell/platform/linux/testing/atspi_test_bus.h"

namespace {

constexpr char kRootPath[] = "/org/a11y/atspi/accessible/root";
constexpr char kAccessibleInterface[] = "org.a11y.atspi.Accessible";

}  // namespace

G_DECLARE_FINAL_TYPE(FlTestWindow, fl_test_window, FL, TEST_WINDOW, GObject);

// A window with a fixed position and size.
struct _FlTestWindow {
  GObject parent_instance;
  FlAtspiApplication* application;
};

static void fl_test_window_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlTestWindow,
    fl_test_window,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(fl_atspi_accessible_get_type(),
                          fl_test_window_atspi_accessible_iface_init))

static FlAtspiAccessible* fl_test_window_get_parent(
    FlAtspiAccessible* accessible) {
  return FL_ATSPI_ACCESSIBLE(FL_TEST_WINDOW(accessible)->application);
}

static gsize fl_test_window_get_n_children(FlAtspiAccessible* accessible) {
  return 0;
}

static FlAtspiAccessible* fl_test_window_get_child(
    FlAtspiAccessible* accessible,
    gsize index) {
  return nullptr;
}

static FlAtspiRole fl_test_window_get_role(FlAtspiAccessible* accessible) {
  return FL_ATSPI_ROLE_PANEL;
}

static gchar* fl_test_window_get_name(FlAtspiAccessible* accessible) {
  return g_strdup("Test Window");
}

static guint64 fl_test_window_get_states(FlAtspiAccessible* accessible) {
  return FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_SHOWING);
}

static gboolean fl_test_window_get_extents(FlAtspiAccessible* accessible,
                                           GdkRectangle* extents) {
  *extents = {0, 0, 640, 480};
  return TRUE;
}

static void fl_test_window_get_origin(FlAtspiAccessible* accessible,
                                      FlAtspiCoordType coord_type,
                                      gint* x,
                                      gint* y) {
  *x = coord_type == FL_ATSPI_COORD_TYPE_SCREEN ? 100 : 10;
  *y = coord_type == FL_ATSPI_COORD_TYPE_SCREEN ? 200 : 20;
}

static void fl_test_window_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface) {
  iface->get_parent = fl_test_window_get_parent;
  iface->get_n_children = fl_test_window_get_n_children;
  iface->get_child = fl_test_window_get_child;
  iface->get_role = fl_test_window_get_role;
  iface->get_name = fl_test_window_get_name;
  iface->get_states = fl_test_window_get_states;
  iface->get_extents = fl_test_window_get_extents;
  iface->get_origin = fl_test_window_get_origin;
}

static void fl_test_window_dispose(GObject* object) {
  FlTestWindow* self = FL_TEST_WINDOW(object);
  if (self->application != nullptr) {
    fl_atspi_application_remove_window(self->application,
                                       FL_ATSPI_ACCESSIBLE(self));
  }
  g_clear_object(&self->application);
  G_OBJECT_CLASS(fl_test_window_parent_class)->dispose(object);
}

static void fl_test_window_class_init(FlTestWindowClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_test_window_dispose;
}

static void fl_test_window_init(FlTestWindow* self) {}

static FlTestWindow* fl_test_window_new(FlAtspiApplication* application) {
  FlTestWindow* self =
      FL_TEST_WINDOW(g_object_new(fl_test_window_get_type(), nullptr));
  self->application = FL_ATSPI_APPLICATION(g_object_ref(application));
  fl_atspi_application_add_window(application, FL_ATSPI_ACCESSIBLE(self));
  return self;
}

namespace {

class FlAtspiApplicationTest : public flutter::testing::LinuxTest {
 protected:
  void SetUp() override {
    g_set_prgname("test-app");
    bus = std::make_unique<flutter::testing::AtspiTestBus>();

    // ATK only records the plug ID of sockets when it has an implementation,
    // which is normally provided by the AT-SPI bridge.
    socket_class = ATK_SOCKET_CLASS(g_type_class_ref(ATK_TYPE_SOCKET));
    saved_embed = socket_class->embed;
    socket_class->embed = [](AtkSocket* socket, const gchar* plug_id) {};
  }

  void TearDown() override {
    socket_class->embed = saved_embed;
    g_type_class_unref(socket_class);
    g_clear_object(&window);
    g_clear_object(&application);
    bus.reset();
    LinuxTest::TearDown();
  }

  // Creates an application with a window on the test bus, with the registry
  // reporting |events| as registered.
  void Connect(const std::vector<std::string>& events) {
    bus->ServeRegistry(events);
    application =
        fl_atspi_application_new_with_connection(bus->app_connection());
    window = fl_test_window_new(application);
    ASSERT_TRUE(flutter::testing::AtspiTestBus::RunUntil(
        [this]() { return bus->registry_called(); }));
    // A round trip ensures the registered events have been received.
    bus->Sync();
    window_path = bus->GetChildPath(kRootPath, 0);
  }

  std::unique_ptr<flutter::testing::AtspiTestBus> bus;
  FlAtspiApplication* application = nullptr;
  FlTestWindow* window = nullptr;
  std::string window_path;
  AtkSocketClass* socket_class = nullptr;
  void (*saved_embed)(AtkSocket* socket, const gchar* plug_id) = nullptr;
};

}  // namespace

TEST_F(FlAtspiApplicationTest, NotConnected) {
  g_autoptr(FlAtspiApplication) unconnected = fl_atspi_application_new();
  g_autoptr(FlTestWindow) unconnected_window = fl_test_window_new(unconnected);
  g_autoptr(AtkSocket) socket = ATK_SOCKET(atk_socket_new());
  fl_atspi_application_set_window_socket(
      unconnected, FL_ATSPI_ACCESSIBLE(unconnected_window), socket);
  EXPECT_EQ(socket->embedded_plug_id, nullptr);
}

TEST_F(FlAtspiApplicationTest, Connects) {
  Connect({});
  g_autoptr(AtkSocket) socket = ATK_SOCKET(atk_socket_new());
  fl_atspi_application_set_window_socket(application,
                                         FL_ATSPI_ACCESSIBLE(window), socket);
  // Peer-to-peer connections have no bus name.
  EXPECT_STREQ(socket->embedded_plug_id, (":" + window_path).c_str());

  // Not embedded once disconnected.
  bool closed = false;
  g_signal_connect_swapped(bus->app_connection(), "closed",
                           G_CALLBACK(+[](bool* closed) { *closed = true; }),
                           &closed);
  g_dbus_connection_close_sync(bus->app_connection(), nullptr, nullptr);
  EXPECT_TRUE(
      flutter::testing::AtspiTestBus::RunUntil([&closed]() { return closed; }));
  g_clear_pointer(&socket->embedded_plug_id, g_free);
  fl_atspi_application_set_window_socket(application,
                                         FL_ATSPI_ACCESSIBLE(window), socket);
  EXPECT_EQ(socket->embedded_plug_id, nullptr);
}

TEST_F(FlAtspiApplicationTest, EmbedKeepsSocketState) {
  Connect({});
  // Simulate at-spi2-atk embedding.
  socket_class->embed = [](AtkSocket* socket, const gchar* plug_id) {
    ATK_OBJECT_GET_CLASS(socket)->ref_state_set = [](AtkObject* object) {
      return atk_state_set_new();
    };
  };
  g_autoptr(AtkSocket) socket = ATK_SOCKET(atk_socket_new());
  AtkObjectClass* klass = ATK_OBJECT_GET_CLASS(socket);
  auto ref_state_set = klass->ref_state_set;
  fl_atspi_application_set_window_socket(application,
                                         FL_ATSPI_ACCESSIBLE(window), socket);
  EXPECT_EQ(klass->ref_state_set, ref_state_set);
  klass->ref_state_set = ref_state_set;
}

TEST_F(FlAtspiApplicationTest, RootObject) {
  Connect({});

  GVariant* reply = bus->Call(kRootPath, kAccessibleInterface, "GetChildren");
  ASSERT_NE(reply, nullptr);
  ASSERT_STREQ(g_variant_get_type_string(reply), "(a(so))");
  // Windows from other tests may also exist, as the application is shared.
  g_autoptr(GVariantIter) iter = nullptr;
  g_variant_get(reply, "(a(so))", &iter);
  const gchar* name;
  const gchar* path;
  bool found = false;
  while (g_variant_iter_next(iter, "(&s&o)", &name, &path)) {
    if (path == window_path) {
      EXPECT_STREQ(name, "");
      found = true;
    }
  }
  EXPECT_TRUE(found);

  GVariant* value = bus->GetProperty(kRootPath, kAccessibleInterface, "Name");
  ASSERT_NE(value, nullptr);
  EXPECT_STREQ(g_variant_get_string(value, nullptr), "test-app");

  reply = bus->Call(kRootPath, kAccessibleInterface, "GetInterfaces");
  ASSERT_NE(reply, nullptr);
  g_autofree const gchar** interfaces = nullptr;
  g_variant_get(reply, "(^a&s)", &interfaces);
  ASSERT_EQ(g_strv_length(const_cast<gchar**>(interfaces)), 2u);
  EXPECT_STREQ(interfaces[0], kAccessibleInterface);
  EXPECT_STREQ(interfaces[1], "org.a11y.atspi.Application");

  reply = bus->Call(kRootPath, "org.a11y.atspi.Application",
                    "GetApplicationBusAddress");
  ASSERT_NE(reply, nullptr);
  const gchar* address;
  g_variant_get(reply, "(&s)", &address);
  EXPECT_STREQ(address, "");

  reply =
      bus->Call("/org/a11y/atspi/cache", "org.a11y.atspi.Cache", "GetItems");
  ASSERT_NE(reply, nullptr);
  EXPECT_STREQ(g_variant_get_type_string(reply), "(a((so)(so)(so)iiassusau))");
}

TEST_F(FlAtspiApplicationTest, WindowObject) {
  Connect({});

  GVariant* reply = bus->Call(window_path, kAccessibleInterface, "GetRole");
  ASSERT_NE(reply, nullptr);
  uint32_t role;
  g_variant_get(reply, "(u)", &role);
  EXPECT_EQ(role, static_cast<uint32_t>(FL_ATSPI_ROLE_PANEL));

  reply = bus->Call(window_path, "org.freedesktop.DBus.Properties", "GetAll",
                    g_variant_new("(s)", kAccessibleInterface));
  ASSERT_NE(reply, nullptr);
  g_autoptr(GVariant) properties = g_variant_get_child_value(reply, 0);
  const gchar* title;
  ASSERT_TRUE(g_variant_lookup(properties, "Name", "&s", &title));
  EXPECT_STREQ(title, "Test Window");

  // Window coordinates.
  reply = bus->Call(window_path, "org.a11y.atspi.Component", "GetExtents",
                    g_variant_new("(u)", 1));
  ASSERT_NE(reply, nullptr);
  int32_t x, y, width, height;
  g_variant_get(reply, "((iiii))", &x, &y, &width, &height);
  EXPECT_EQ(x, 10);
  EXPECT_EQ(y, 20);
  EXPECT_EQ(width, 640);
  EXPECT_EQ(height, 480);

  // Screen coordinates.
  reply = bus->Call(window_path, "org.a11y.atspi.Component", "GetExtents",
                    g_variant_new("(u)", 0));
  ASSERT_NE(reply, nullptr);
  g_variant_get(reply, "((iiii))", &x, &y, &width, &height);
  EXPECT_EQ(x, 100);
  EXPECT_EQ(y, 200);

  reply = bus->Call(window_path, "org.a11y.atspi.Component", "Contains",
                    g_variant_new("(iiu)", 101, 201, 0));
  ASSERT_NE(reply, nullptr);
  gboolean contains;
  g_variant_get(reply, "(b)", &contains);
  EXPECT_TRUE(contains);
  reply = bus->Call(window_path, "org.a11y.atspi.Component", "Contains",
                    g_variant_new("(iiu)", 99, 201, 0));
  ASSERT_NE(reply, nullptr);
  g_variant_get(reply, "(b)", &contains);
  EXPECT_FALSE(contains);
}

TEST_F(FlAtspiApplicationTest, Embedded) {
  Connect({});

  GVariant* parent =
      bus->GetProperty(window_path, kAccessibleInterface, "Parent");
  ASSERT_NE(parent, nullptr);
  const gchar* name;
  const gchar* path;
  g_variant_get(parent, "(&s&o)", &name, &path);
  EXPECT_STREQ(path, kRootPath);

  ASSERT_NE(bus->Call(window_path, "org.a11y.atspi.Socket", "Embedded",
                      g_variant_new("(s)", "/org/a11y/atspi/accessible/42")),
            nullptr);

  // The window is now a child of the socket.
  parent = bus->GetProperty(window_path, kAccessibleInterface, "Parent");
  ASSERT_NE(parent, nullptr);
  g_variant_get(parent, "(&s&o)", &name, &path);
  EXPECT_STREQ(path, "/org/a11y/atspi/accessible/42");
  GVariant* reply =
      bus->Call(window_path, kAccessibleInterface, "GetIndexInParent");
  ASSERT_NE(reply, nullptr);
  int32_t index;
  g_variant_get(reply, "(i)", &index);
  EXPECT_EQ(index, 0);
}

TEST_F(FlAtspiApplicationTest, SelectionAndImage) {
  Connect({});
  g_autoptr(FlViewAccessible) view =
      fl_view_accessible_new(engine, application, 456);
  std::string view_path = bus->GetChildPath(kRootPath, 1);

  FlutterSemanticsFlags flags[3] = {};
  FlutterSemanticsNode2 nodes[3] = {};
  int32_t children[2] = {1, 2};
  for (int i = 0; i < 3; i++) {
    flags[i].struct_size = sizeof(FlutterSemanticsFlags);
    nodes[i].struct_size = sizeof(FlutterSemanticsNode2);
    nodes[i].id = i;
    nodes[i].flags2 = &flags[i];
    nodes[i].rect = {0, 0, 10, 20};
    nodes[i].transform.scaleX = 1;
    nodes[i].transform.scaleY = 1;
    nodes[i].transform.pers2 = 1;
  }
  nodes[0].child_count = 2;
  nodes[0].children_in_traversal_order = children;
  nodes[0].children_in_hit_test_order = children;
  flags[1].is_selected = kFlutterTristateFalse;
  flags[2].is_selected = kFlutterTristateTrue;
  flags[2].is_image = true;
  nodes[2].label = "Logo";
  FlutterSemanticsNode2* node_pointers[3] = {&nodes[0], &nodes[1], &nodes[2]};
  FlutterSemanticsUpdate2 update = {};
  update.struct_size = sizeof(update);
  update.node_count = 3;
  update.nodes = node_pointers;
  fl_view_accessible_handle_update_semantics(view, &update);

  std::string container = bus->GetChildPath(view_path, 0);
  std::string unselected = bus->GetChildPath(container, 0);
  std::string image = bus->GetChildPath(container, 1);
  GVariant* reply = bus->Call(container, "org.a11y.atspi.Selection",
                              "GetSelectedChild", g_variant_new("(i)", 0));
  ASSERT_NE(reply, nullptr);
  const gchar* name;
  const gchar* path;
  g_variant_get(reply, "((&s&o))", &name, &path);
  EXPECT_EQ(path, image);

  GVariant* count = bus->GetProperty(container, "org.a11y.atspi.Selection",
                                     "NSelectedChildren");
  ASSERT_NE(count, nullptr);
  EXPECT_EQ(g_variant_get_int32(count), 1);

  reply = bus->Call(image, "org.a11y.atspi.Image", "GetImageSize");
  ASSERT_NE(reply, nullptr);
  int32_t width, height;
  g_variant_get(reply, "(ii)", &width, &height);
  EXPECT_EQ(width, 10);
  EXPECT_EQ(height, 20);

  // Nodes without an image don't implement the interface.
  g_autoptr(GError) error = nullptr;
  EXPECT_EQ(bus->Call(unselected, "org.a11y.atspi.Image", "GetImageSize",
                      nullptr, &error),
            nullptr);
  EXPECT_NE(error, nullptr);
}

TEST_F(FlAtspiApplicationTest, UnknownObject) {
  Connect({});
  g_autoptr(GError) error = nullptr;
  EXPECT_EQ(bus->Call("/org/flutter/a11y/99", kAccessibleInterface, "GetRole",
                      nullptr, &error),
            nullptr);
  EXPECT_NE(error, nullptr);
}

TEST_F(FlAtspiApplicationTest, FiltersEvents) {
  Connect({"Object:Announcement"});

  // Not registered for, so not sent.
  fl_atspi_application_emit_text_caret_moved(application,
                                             FL_ATSPI_ACCESSIBLE(window), 1);
  fl_atspi_application_emit_announcement(
      application, FL_ATSPI_ACCESSIBLE(window), "Hello", FALSE);
  bus->Sync();

  ASSERT_EQ(bus->signals().size(), 1u);
  const auto& signal = bus->signals()[0];
  EXPECT_EQ(signal.path, window_path);
  EXPECT_EQ(signal.interface, "org.a11y.atspi.Event.Object");
  EXPECT_EQ(signal.member, "Announcement");
  ASSERT_STREQ(g_variant_get_type_string(signal.parameters), "(siiva{sv})");
  const gchar* detail;
  int32_t detail1, detail2;
  g_autoptr(GVariant) data = nullptr;
  g_variant_get(signal.parameters, "(&siiva{sv})", &detail, &detail1, &detail2,
                &data, nullptr);
  EXPECT_EQ(detail1, 1);
  EXPECT_STREQ(g_variant_get_string(data, nullptr), "Hello");
}

TEST_F(FlAtspiApplicationTest, ListenerRegistered) {
  Connect({});
  bus->EmitSignal(
      "/org/a11y/atspi/registry", "org.a11y.atspi.Registry",
      "EventListenerRegistered",
      g_variant_new("(ssas)", ":1.9", "Object:TextCaretMoved", nullptr));
  // A round trip ensures the signal has been handled.
  bus->Sync();

  // Caret events are now sent, other events except cache updates aren't.
  fl_atspi_application_emit_announcement(
      application, FL_ATSPI_ACCESSIBLE(window), "Ignored", FALSE);
  fl_atspi_application_emit_text_caret_moved(application,
                                             FL_ATSPI_ACCESSIBLE(window), 3);
  bus->Sync();
  ASSERT_EQ(bus->signals().size(), 1u);
  EXPECT_EQ(bus->signals()[0].interface, "org.a11y.atspi.Event.Object");
  EXPECT_EQ(bus->signals()[0].member, "TextCaretMoved");
}
