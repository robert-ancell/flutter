// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_windowing_handler.h"
#include "flutter/shell/platform/linux/testing/mock_gtk.h"
#include "flutter/testing/testing.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(FlWindowingHandlerTest, CreateRegular) {}

TEST(FlWindowingHandlerTest, CreateRegularMinSize) {}

TEST(FlWindowingHandlerTest, CreateRegularMaxSize) {}

TEST(FlWindowingHandlerTest, CreateRegularWithTitle) {}

TEST(FlWindowingHandlerTest, CreateRegularMinimized) {}

TEST(FlWindowingHandlerTest, ModifyRegularSize) {}

TEST(FlWindowingHandlerTest, ModifyRegularTitle) {}

TEST(FlWindowingHandlerTest, ModifyRegularMaximize) {}

TEST(FlWindowingHandlerTest, ModifyRegularUnmaximize) {}

TEST(FlWindowingHandlerTest, ModifyRegularMinimize) {}

TEST(FlWindowingHandlerTest, ModifyRegularUnminimize) {}

TEST(FlWindowingHandlerTest, ModifyUnknownWindow) {}

TEST(FlWindowingHandlerTest, DestroyWindow) {}

TEST(FlWindowingHandlerTest, DestroyUnknownWindow) {}
