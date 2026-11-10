// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_accessibility_handler.h"
#include "flutter/shell/platform/linux/fl_binary_messenger_private.h"
#include "flutter/shell/platform/linux/fl_engine_private.h"
#include "flutter/shell/platform/linux/fl_view_private.h"
#include "flutter/shell/platform/linux/testing/fl_mock_binary_messenger.h"
#include "flutter/shell/platform/linux/testing/fl_test_gtk_logs.h"
#include "flutter/testing/testing.h"

#include "flutter/shell/platform/linux/testing/linux_test.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

static constexpr int64_t kTextDirectionLtr = 0;
static constexpr int64_t kAssertivenessAssertive = 1;

class FlAccessibilityHandlerTest : public flutter::testing::LinuxTest {
 protected:
  void SetUp() override {
    flutter::testing::fl_ensure_gtk_init();
    messenger = fl_mock_binary_messenger_new();
    g_clear_object(&engine);
    engine =
        fl_engine_new_with_binary_messenger(FL_BINARY_MESSENGER(messenger));
    view = fl_view_new_for_engine(engine);
  }

  ~FlAccessibilityHandlerTest() {
    fl_binary_messenger_shutdown(FL_BINARY_MESSENGER(messenger));
    g_clear_object(&messenger);
  }

  FlMockBinaryMessenger* messenger = nullptr;
  FlView* view = nullptr;
};

TEST_F(FlAccessibilityHandlerTest, Announce) {
  g_autoptr(FlValue) message = fl_value_new_map();
  fl_value_set_string_take(message, "type", fl_value_new_string("announce"));
  g_autoptr(FlValue) data = fl_value_new_map();
  fl_value_set_string(message, "data", data);
  fl_value_set_string_take(data, "viewId",
                           fl_value_new_int(fl_view_get_id(view)));
  fl_value_set_string_take(data, "message", fl_value_new_string("MESSAGE"));
  fl_value_set_string_take(data, "textDirection",
                           fl_value_new_int(kTextDirectionLtr));

  gboolean called = FALSE;
  fl_mock_binary_messenger_send_standard_message(
      messenger, "flutter/accessibility", message,
      [](FlMockBinaryMessenger* messenger, FlValue* response,
         gpointer user_data) {
        EXPECT_EQ(fl_value_get_type(response), FL_VALUE_TYPE_NULL);
        gboolean* called = static_cast<gboolean*>(user_data);
        *called = TRUE;
      },
      &called);
  EXPECT_TRUE(called);
}

TEST_F(FlAccessibilityHandlerTest, AnnounceAssertive) {
  g_autoptr(FlValue) message = fl_value_new_map();
  fl_value_set_string_take(message, "type", fl_value_new_string("announce"));
  g_autoptr(FlValue) data = fl_value_new_map();
  fl_value_set_string(message, "data", data);
  fl_value_set_string_take(data, "viewId",
                           fl_value_new_int(fl_view_get_id(view)));
  fl_value_set_string_take(data, "message", fl_value_new_string("MESSAGE"));
  fl_value_set_string_take(data, "textDirection",
                           fl_value_new_int(kTextDirectionLtr));
  // Add optional assertiveness.
  fl_value_set_string_take(data, "assertiveness",
                           fl_value_new_int(kAssertivenessAssertive));

  gboolean called = FALSE;
  fl_mock_binary_messenger_send_standard_message(
      messenger, "flutter/accessibility", message,
      [](FlMockBinaryMessenger* messenger, FlValue* response,
         gpointer user_data) {
        EXPECT_EQ(fl_value_get_type(response), FL_VALUE_TYPE_NULL);
        gboolean* called = static_cast<gboolean*>(user_data);
        *called = TRUE;
      },
      &called);
  EXPECT_TRUE(called);
}

TEST_F(FlAccessibilityHandlerTest, AnnounceUnknownView) {
  g_autoptr(FlValue) message = fl_value_new_map();
  fl_value_set_string_take(message, "type", fl_value_new_string("announce"));
  g_autoptr(FlValue) data = fl_value_new_map();
  fl_value_set_string(message, "data", data);
  fl_value_set_string_take(data, "viewId", fl_value_new_int(999));
  fl_value_set_string_take(data, "message", fl_value_new_string("MESSAGE"));
  fl_value_set_string_take(data, "textDirection",
                           fl_value_new_int(kTextDirectionLtr));
  // Add optional assertiveness.
  fl_value_set_string_take(data, "assertiveness",
                           fl_value_new_int(kAssertivenessAssertive));

  gboolean called = FALSE;
  fl_mock_binary_messenger_send_standard_message(
      messenger, "flutter/accessibility", message,
      [](FlMockBinaryMessenger* messenger, FlValue* response,
         gpointer user_data) {
        EXPECT_EQ(fl_value_get_type(response), FL_VALUE_TYPE_NULL);
        gboolean* called = static_cast<gboolean*>(user_data);
        *called = TRUE;
      },
      &called);
  EXPECT_TRUE(called);
}

TEST_F(FlAccessibilityHandlerTest, UnknownType) {
  // Unknown type, ignored by embedder.
  g_autoptr(FlValue) message = fl_value_new_map();
  fl_value_set_string_take(message, "type", fl_value_new_string("UNKNOWN"));
  fl_value_set_string_take(message, "data", fl_value_new_map());

  gboolean called = FALSE;
  fl_mock_binary_messenger_send_standard_message(
      messenger, "flutter/accessibility", message,
      [](FlMockBinaryMessenger* messenger, FlValue* response,
         gpointer user_data) {
        EXPECT_EQ(fl_value_get_type(response), FL_VALUE_TYPE_NULL);
        gboolean* called = static_cast<gboolean*>(user_data);
        *called = TRUE;
      },
      &called);
  EXPECT_TRUE(called);
}
