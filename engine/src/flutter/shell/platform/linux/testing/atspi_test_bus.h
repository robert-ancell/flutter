// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_TESTING_ATSPI_TEST_BUS_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_TESTING_ATSPI_TEST_BUS_H_

#include <gio/gio.h>

#include <map>
#include <string>
#include <vector>

namespace flutter {
namespace testing {

// A peer-to-peer D-Bus connection that acts as the accessibility bus in tests.
//
// The application side is passed to fl_atspi_application_new_with_connection()
// and the test makes calls and receives signals on the peer side.
class AtspiTestBus {
 public:
  struct Signal {
    std::string path;
    std::string interface;
    std::string member;
    GVariant* parameters;
  };

  AtspiTestBus();
  ~AtspiTestBus();

  // The connection the application should use.
  GDBusConnection* app_connection() const { return app_connection_; }

  // Serves the AT-SPI registry, reporting |events| as registered. Must be
  // called before the application connects.
  void ServeRegistry(const std::vector<std::string>& events);

  // TRUE if the application has requested the registered events.
  bool registry_called() const { return registry_called_; }

  // Calls a method on the application and returns the reply, or nullptr on
  // error. The reply is owned by this object.
  GVariant* Call(const std::string& path,
                 const std::string& interface,
                 const std::string& member,
                 GVariant* parameters = nullptr,
                 GError** error = nullptr);

  // Gets a property of an object in the application, or nullptr on error.
  GVariant* GetProperty(const std::string& path,
                        const std::string& interface,
                        const std::string& name);

  // Gets the path of the child at |index| of the object at |path|, or an empty
  // string on error.
  std::string GetChildPath(const std::string& path, int index);

  // Makes a round trip to the application, so any signals it emitted before
  // have been received.
  void Sync();

  // Emits a signal to the application.
  void EmitSignal(const std::string& path,
                  const std::string& interface,
                  const std::string& member,
                  GVariant* parameters);

  const std::vector<Signal>& signals() const { return signals_; }

  void ClearSignals();

  // Received signals formatted as "path member:detail detail1", with paths
  // found in |labels| replaced with their label.
  std::vector<std::string> Describe(
      const std::map<std::string, std::string>& labels);

  // Iterates the main loop until |done| returns true. Returns false on
  // timeout.
  template <typename F>
  static bool RunUntil(F done) {
    gint64 deadline = g_get_monotonic_time() + 5 * G_USEC_PER_SEC;
    while (!done()) {
      if (g_get_monotonic_time() > deadline) {
        return false;
      }
      g_main_context_iteration(nullptr, FALSE);
    }
    return true;
  }

 private:
  GDBusConnection* peer_ = nullptr;
  GDBusConnection* app_connection_ = nullptr;
  guint signal_subscription_ = 0;
  guint registry_id_ = 0;
  GDBusNodeInfo* registry_info_ = nullptr;
  std::vector<std::string> registered_events_;
  bool registry_called_ = false;
  std::vector<Signal> signals_;
  std::vector<GVariant*> replies_;
};

}  // namespace testing
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_TESTING_ATSPI_TEST_BUS_H_
