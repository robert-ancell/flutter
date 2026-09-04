// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_pointer_manager.h"
#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/linux/fl_engine_private.h"

#include <gdk/gdkwayland.h>

#include "flutter/shell/platform/linux/testing/linux_test.h"
#include "gtest/gtest.h"

class FlPointerManagerTest : public flutter::testing::LinuxTest {};

// Creates a device that generates events from the given input source.
static GdkDevice* make_device(GdkInputSource source) {
  return GDK_DEVICE(g_object_new(gdk_wayland_device_get_type(), "input-source",
                                 source, nullptr));
}

TEST_F(FlPointerManagerTest, EnterLeave) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_enter(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_leave(manager, 1235, mouse, 3.0, 4.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 2u);

  EXPECT_EQ(pointer_events[0].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[0].x, 1.0);
  EXPECT_EQ(pointer_events[0].y, 2.0);
  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[0].buttons, 0);
  EXPECT_EQ(pointer_events[0].view_id, 42);

  EXPECT_EQ(pointer_events[1].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[1].x, 3.0);
  EXPECT_EQ(pointer_events[1].y, 4.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, 0);
  EXPECT_EQ(pointer_events[1].view_id, 42);
}

TEST_F(FlPointerManagerTest, EnterEnter) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_enter(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  // Duplicate enter is ignored
  fl_pointer_manager_handle_enter(manager, 1235, mouse, 3.0, 4.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 1u);

  EXPECT_EQ(pointer_events[0].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[0].x, 1.0);
  EXPECT_EQ(pointer_events[0].y, 2.0);
  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[0].buttons, 0);
  EXPECT_EQ(pointer_events[0].view_id, 42);
}

TEST_F(FlPointerManagerTest, EnterLeaveLeave) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_enter(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_leave(manager, 1235, mouse, 3.0, 4.0, 0, 0);
  // Duplicate leave is ignored
  fl_pointer_manager_handle_leave(manager, 1235, mouse, 5.0, 6.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 2u);

  EXPECT_EQ(pointer_events[0].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[0].x, 1.0);
  EXPECT_EQ(pointer_events[0].y, 2.0);
  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[0].buttons, 0);
  EXPECT_EQ(pointer_events[0].view_id, 42);

  EXPECT_EQ(pointer_events[1].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[1].x, 3.0);
  EXPECT_EQ(pointer_events[1].y, 4.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, 0);
  EXPECT_EQ(pointer_events[1].view_id, 42);
}

TEST_F(FlPointerManagerTest, EnterButtonPress) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_enter(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 2u);

  EXPECT_EQ(pointer_events[0].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[0].x, 1.0);
  EXPECT_EQ(pointer_events[0].y, 2.0);
  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[0].buttons, 0);
  EXPECT_EQ(pointer_events[0].view_id, 42);

  EXPECT_EQ(pointer_events[1].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
}

TEST_F(FlPointerManagerTest, NoEnterButtonPress) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 2u);

  // Synthetic enter events
  EXPECT_EQ(pointer_events[0].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[0].x, 4.0);
  EXPECT_EQ(pointer_events[0].y, 8.0);
  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[0].buttons, 0);
  EXPECT_EQ(pointer_events[0].view_id, 42);

  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleasePrimary) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseSecondary) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_SECONDARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_SECONDARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMouseSecondary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseMiddle) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_MIDDLE, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_MIDDLE, 0, 0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMouseMiddle);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseBack) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  // Forward button is 8 (no GDK define).
  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0, 8, 0,
                                         0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0, 8, 0,
                                           0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMouseBack);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseForward) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  // Forward button is 9 (no GDK define).
  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0, 9, 0,
                                         0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0, 9, 0,
                                           0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMouseForward);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseThreeButtons) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  // Press buttons 1-2-3, release 3-2-1
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 1.0, 2.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, mouse, 3.0, 4.0,
                                         GDK_BUTTON_SECONDARY, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1236, mouse, 5.0, 6.0,
                                         GDK_BUTTON_MIDDLE, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1237, mouse, 7.0, 8.0,
                                           GDK_BUTTON_MIDDLE, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1238, mouse, 9.0, 10.0,
                                           GDK_BUTTON_SECONDARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1239, mouse, 11.0, 12.0,
                                           kFlutterPointerButtonMousePrimary, 0,
                                           0);

  EXPECT_EQ(pointer_events.size(), 7u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 1.0);
  EXPECT_EQ(pointer_events[1].y, 2.0);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 3.0);
  EXPECT_EQ(pointer_events[2].y, 4.0);
  EXPECT_EQ(pointer_events[2].buttons, kFlutterPointerButtonMousePrimary |
                                           kFlutterPointerButtonMouseSecondary);
  EXPECT_EQ(pointer_events[3].timestamp, 1236000u);
  EXPECT_EQ(pointer_events[3].x, 5.0);
  EXPECT_EQ(pointer_events[3].y, 6.0);
  EXPECT_EQ(pointer_events[3].buttons, kFlutterPointerButtonMousePrimary |
                                           kFlutterPointerButtonMouseSecondary |
                                           kFlutterPointerButtonMouseMiddle);
  EXPECT_EQ(pointer_events[4].timestamp, 1237000u);
  EXPECT_EQ(pointer_events[4].x, 7.0);
  EXPECT_EQ(pointer_events[4].y, 8.0);
  EXPECT_EQ(pointer_events[4].buttons, kFlutterPointerButtonMousePrimary |
                                           kFlutterPointerButtonMouseSecondary);
  EXPECT_EQ(pointer_events[5].timestamp, 1238000u);
  EXPECT_EQ(pointer_events[5].x, 9.0);
  EXPECT_EQ(pointer_events[5].y, 10.0);
  EXPECT_EQ(pointer_events[5].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[6].timestamp, 1239000u);
  EXPECT_EQ(pointer_events[6].x, 11.0);
  EXPECT_EQ(pointer_events[6].y, 12.0);
  EXPECT_EQ(pointer_events[6].buttons, 0);
}

TEST_F(FlPointerManagerTest, ButtonPressStylusPrimaryButton) {
  StartEngine();

  constexpr int64_t kStylusContact = 1 << 0;
  constexpr int64_t kStylusPrimary = 1 << 1;

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* stylus = make_device(GDK_SOURCE_PEN);
  fl_pointer_manager_handle_button_press(manager, 1234, stylus, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0.0, 0.5);
  fl_pointer_manager_handle_button_press(manager, 1235, stylus, 4.0, 8.0,
                                         GDK_BUTTON_SECONDARY, 0.0, 0.5);

  EXPECT_EQ(pointer_events.size(), 3u);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[2].buttons, kStylusContact | kStylusPrimary);
}

TEST_F(FlPointerManagerTest, ButtonPressStylusContact) {
  StartEngine();

  constexpr int64_t kStylusContact = 1 << 0;

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* stylus = make_device(GDK_SOURCE_PEN);
  fl_pointer_manager_handle_button_press(manager, 1234, stylus, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0.0, 0.5);

  EXPECT_EQ(pointer_events.size(), 2u);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[1].buttons, kStylusContact);
}

TEST_F(FlPointerManagerTest, ButtonPressInvertedStylusContact) {
  StartEngine();

  constexpr int64_t kStylusContact = 1 << 0;

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* eraser = make_device(GDK_SOURCE_ERASER);
  fl_pointer_manager_handle_button_press(manager, 1234, eraser, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0.0, 0.5);

  EXPECT_EQ(pointer_events.size(), 2u);
  EXPECT_EQ(pointer_events[1].device_kind,
            kFlutterPointerDeviceKindInvertedStylus);
  EXPECT_EQ(pointer_events[1].buttons, kStylusContact);
}

TEST_F(FlPointerManagerTest, ButtonPressStylusSecondaryButton) {
  StartEngine();

  constexpr int64_t kStylusContact = 1 << 0;
  constexpr int64_t kStylusSecondary = 1 << 2;

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* stylus = make_device(GDK_SOURCE_PEN);
  fl_pointer_manager_handle_button_press(manager, 1234, stylus, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0.0, 0.5);
  fl_pointer_manager_handle_button_press(manager, 1235, stylus, 4.0, 8.0,
                                         GDK_BUTTON_MIDDLE, 0.0, 0.5);

  EXPECT_EQ(pointer_events.size(), 3u);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[2].buttons, kStylusContact | kStylusSecondary);
}

TEST_F(FlPointerManagerTest, ButtonPressStylusUnknownButton) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* stylus = make_device(GDK_SOURCE_PEN);
  fl_pointer_manager_handle_button_press(manager, 1234, stylus, 4.0, 8.0, 8, 0,
                                         0);

  EXPECT_EQ(pointer_events.size(), 0u);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonPressButtonRelease) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  // A second press without a release means the release was lost, e.g. it went
  // to the window manager at the end of an interactive move. The stale press
  // is cancelled and this press is handled.
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 6.0, 10.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 5u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].phase, kDown);
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].phase, kCancel);
  EXPECT_EQ(pointer_events[2].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[2].x, 6.0);
  EXPECT_EQ(pointer_events[2].y, 10.0);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
  EXPECT_EQ(pointer_events[3].phase, kDown);
  EXPECT_EQ(pointer_events[3].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[3].x, 6.0);
  EXPECT_EQ(pointer_events[3].y, 10.0);
  EXPECT_EQ(pointer_events[3].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[3].view_id, 42);
  EXPECT_EQ(pointer_events[4].phase, kUp);
  EXPECT_EQ(pointer_events[4].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[4].x, 5.0);
  EXPECT_EQ(pointer_events[4].y, 9.0);
  EXPECT_EQ(pointer_events[4].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[4].buttons, 0);
  EXPECT_EQ(pointer_events[4].view_id, 42);
}

TEST_F(FlPointerManagerTest, GrabBroken) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1235, mouse, 5.0, 9.0, 0, 0);
  // The grab was taken by the window manager, e.g. an interactive move
  // started. The press is cancelled at the last known position.
  EXPECT_TRUE(fl_pointer_manager_handle_grab_broken(manager, 1236));
  // A later press is not dropped.
  fl_pointer_manager_handle_button_press(manager, 1237, mouse, 6.0, 10.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 5u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].phase, kDown);
  EXPECT_EQ(pointer_events[2].phase, kMove);
  EXPECT_EQ(pointer_events[2].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[3].phase, kCancel);
  EXPECT_EQ(pointer_events[3].timestamp, 1236000u);
  EXPECT_EQ(pointer_events[3].x, 5.0);
  EXPECT_EQ(pointer_events[3].y, 9.0);
  EXPECT_EQ(pointer_events[3].buttons, 0);
  EXPECT_EQ(pointer_events[3].view_id, 42);
  EXPECT_EQ(pointer_events[4].phase, kDown);
  EXPECT_EQ(pointer_events[4].timestamp, 1237000u);
  EXPECT_EQ(pointer_events[4].x, 6.0);
  EXPECT_EQ(pointer_events[4].y, 10.0);
  EXPECT_EQ(pointer_events[4].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[4].view_id, 42);
}

TEST_F(FlPointerManagerTest, GrabBrokenAfterLeave) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  // The pointer was dragged out of the view, the remove is delayed until the
  // button is released.
  fl_pointer_manager_handle_leave(manager, 1235, mouse, 5.0, 9.0, 0, 0);
  // The release never arrives as the grab was broken, so the pointer is
  // cancelled and removed.
  EXPECT_TRUE(fl_pointer_manager_handle_grab_broken(manager, 1236));

  EXPECT_EQ(pointer_events.size(), 4u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].phase, kDown);
  EXPECT_EQ(pointer_events[2].phase, kCancel);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[3].phase, kRemove);
  EXPECT_EQ(pointer_events[3].timestamp, 1236000u);
  EXPECT_EQ(pointer_events[3].x, 5.0);
  EXPECT_EQ(pointer_events[3].y, 9.0);
  EXPECT_EQ(pointer_events[3].buttons, 0);
  EXPECT_EQ(pointer_events[3].view_id, 42);

  // The pointer is added again when it returns.
  fl_pointer_manager_handle_enter(manager, 1237, mouse, 6.0, 10.0, 0, 0);
  EXPECT_EQ(pointer_events.size(), 5u);
  EXPECT_EQ(pointer_events[4].phase, kAdd);
}

TEST_F(FlPointerManagerTest, GrabBrokenNoButtons) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_enter(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  // Nothing to cancel if no buttons are pressed.
  EXPECT_FALSE(fl_pointer_manager_handle_grab_broken(manager, 1235));

  EXPECT_EQ(pointer_events.size(), 1u);
}

TEST_F(FlPointerManagerTest, ButtonPressButtonReleaseButtonRelease) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse, 4.0, 8.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);
  // Ignore duplicate release
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 6.0, 10.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 3u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 4.0);
  EXPECT_EQ(pointer_events[1].y, 8.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 5.0);
  EXPECT_EQ(pointer_events[2].y, 9.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
}

TEST_F(FlPointerManagerTest, NoButtonPressButtonRelease) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  // Release without associated press, will be ignored
  fl_pointer_manager_handle_button_release(manager, 1235, mouse, 5.0, 9.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 0u);
}

TEST_F(FlPointerManagerTest, Motion) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_motion(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1235, mouse, 3.0, 4.0, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1236, mouse, 5.0, 6.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 4u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 1.0);
  EXPECT_EQ(pointer_events[1].y, 2.0);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[1].buttons, 0);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 3.0);
  EXPECT_EQ(pointer_events[2].y, 4.0);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[2].buttons, 0);
  EXPECT_EQ(pointer_events[2].view_id, 42);
  EXPECT_EQ(pointer_events[3].timestamp, 1236000u);
  EXPECT_EQ(pointer_events[3].x, 5.0);
  EXPECT_EQ(pointer_events[3].y, 6.0);
  EXPECT_EQ(pointer_events[3].device_kind, kFlutterPointerDeviceKindMouse);
  EXPECT_EQ(pointer_events[3].buttons, 0);
  EXPECT_EQ(pointer_events[3].view_id, 42);
}

TEST_F(FlPointerManagerTest, Drag) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_motion(manager, 1234, mouse, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, mouse, 3.0, 4.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1236, mouse, 5.0, 6.0, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1237, mouse, 7.0, 8.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1238, mouse, 9.0, 10.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 6u);

  // Ignore first synthetic enter event
  EXPECT_EQ(pointer_events[1].timestamp, 1234000u);
  EXPECT_EQ(pointer_events[1].x, 1.0);
  EXPECT_EQ(pointer_events[1].y, 2.0);
  EXPECT_EQ(pointer_events[1].buttons, 0);
  EXPECT_EQ(pointer_events[1].view_id, 42);
  EXPECT_EQ(pointer_events[2].timestamp, 1235000u);
  EXPECT_EQ(pointer_events[2].x, 3.0);
  EXPECT_EQ(pointer_events[2].y, 4.0);
  EXPECT_EQ(pointer_events[2].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[2].view_id, 42);
  EXPECT_EQ(pointer_events[3].timestamp, 1236000u);
  EXPECT_EQ(pointer_events[3].x, 5.0);
  EXPECT_EQ(pointer_events[3].y, 6.0);
  EXPECT_EQ(pointer_events[3].buttons, kFlutterPointerButtonMousePrimary);
  EXPECT_EQ(pointer_events[3].view_id, 42);
  EXPECT_EQ(pointer_events[4].timestamp, 1237000u);
  EXPECT_EQ(pointer_events[4].x, 7.0);
  EXPECT_EQ(pointer_events[4].y, 8.0);
  EXPECT_EQ(pointer_events[4].buttons, 0);
  EXPECT_EQ(pointer_events[4].view_id, 42);
  EXPECT_EQ(pointer_events[5].timestamp, 1238000u);
  EXPECT_EQ(pointer_events[5].x, 9.0);
  EXPECT_EQ(pointer_events[5].y, 10.0);
  EXPECT_EQ(pointer_events[5].buttons, 0);
  EXPECT_EQ(pointer_events[5].view_id, 42);
}

TEST_F(FlPointerManagerTest, DeviceKind) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  // The kind of pointer reported to Flutter comes from the device the events
  // are generated by.
  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* pen = make_device(GDK_SOURCE_PEN);
  fl_pointer_manager_handle_enter(manager, 1234, pen, 1.0, 2.0, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, pen, 1.0, 2.0,
                                         GDK_BUTTON_SECONDARY, 0, 0);
  fl_pointer_manager_handle_motion(manager, 1238, pen, 3.0, 4.0, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1237, pen, 3.0, 4.0,
                                           GDK_BUTTON_SECONDARY, 0, 0);
  fl_pointer_manager_handle_leave(manager, 1235, pen, 3.0, 4.0, 0, 0);

  EXPECT_EQ(pointer_events.size(), 5u);

  EXPECT_EQ(pointer_events[0].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[1].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[2].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[3].device_kind, kFlutterPointerDeviceKindStylus);
  EXPECT_EQ(pointer_events[4].device_kind, kFlutterPointerDeviceKindStylus);
}

TEST_F(FlPointerManagerTest, MultipleDevices) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse1 = make_device(GDK_SOURCE_MOUSE);
  GdkDevice* mouse2 = make_device(GDK_SOURCE_MOUSE);

  // Each device is reported as a separate pointer, and the button pressed on
  // one is not affected by the other being released.
  fl_pointer_manager_handle_button_press(manager, 1234, mouse1, 1.0, 2.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, mouse2, 3.0, 4.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_release(manager, 1236, mouse2, 3.0, 4.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);

  EXPECT_EQ(pointer_events.size(), 5u);

  EXPECT_EQ(pointer_events[0].phase, kAdd);
  EXPECT_EQ(pointer_events[0].device, kMousePointerDeviceId);
  EXPECT_EQ(pointer_events[1].phase, kDown);
  EXPECT_EQ(pointer_events[1].device, kMousePointerDeviceId);
  EXPECT_EQ(pointer_events[1].buttons, kFlutterPointerButtonMousePrimary);

  // The second device gets its own ID, which doesn't collide with the ID used
  // for pan and zoom events.
  EXPECT_EQ(pointer_events[2].phase, kAdd);
  EXPECT_NE(pointer_events[2].device, kMousePointerDeviceId);
  EXPECT_NE(pointer_events[2].device, kPointerPanZoomDeviceId);
  EXPECT_EQ(pointer_events[3].phase, kDown);
  EXPECT_EQ(pointer_events[3].device, pointer_events[2].device);
  EXPECT_EQ(pointer_events[4].phase, kUp);
  EXPECT_EQ(pointer_events[4].device, pointer_events[2].device);
  EXPECT_EQ(pointer_events[4].buttons, 0);

  // The button on the first device is still pressed, so its release is not
  // dropped.
  fl_pointer_manager_handle_button_release(manager, 1237, mouse1, 1.0, 2.0,
                                           GDK_BUTTON_PRIMARY, 0, 0);
  EXPECT_EQ(pointer_events.size(), 6u);
  EXPECT_EQ(pointer_events[5].phase, kUp);
  EXPECT_EQ(pointer_events[5].device, kMousePointerDeviceId);
  EXPECT_EQ(pointer_events[5].buttons, 0);
}

TEST_F(FlPointerManagerTest, MultipleDevicesGrabBroken) {
  StartEngine();

  std::vector<FlutterPointerEvent> pointer_events;
  fl_engine_get_embedder_api(engine)->SendPointerEvent = MOCK_ENGINE_PROC(
      SendPointerEvent,
      ([&pointer_events](auto engine, const FlutterPointerEvent* events,
                         size_t events_count) {
        for (size_t i = 0; i < events_count; i++) {
          pointer_events.push_back(events[i]);
        }

        return kSuccess;
      }));

  g_autoptr(FlPointerManager) manager = fl_pointer_manager_new(42, engine);
  GdkDevice* mouse1 = make_device(GDK_SOURCE_MOUSE);
  GdkDevice* mouse2 = make_device(GDK_SOURCE_MOUSE);
  fl_pointer_manager_handle_button_press(manager, 1234, mouse1, 1.0, 2.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  fl_pointer_manager_handle_button_press(manager, 1235, mouse2, 3.0, 4.0,
                                         GDK_BUTTON_PRIMARY, 0, 0);
  pointer_events.clear();

  // Every device loses its grab.
  EXPECT_TRUE(fl_pointer_manager_handle_grab_broken(manager, 1236));

  EXPECT_EQ(pointer_events.size(), 2u);
  EXPECT_EQ(pointer_events[0].phase, kCancel);
  EXPECT_EQ(pointer_events[1].phase, kCancel);
  EXPECT_NE(pointer_events[0].device, pointer_events[1].device);
}
