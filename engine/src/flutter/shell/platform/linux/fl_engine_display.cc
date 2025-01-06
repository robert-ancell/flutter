// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_engine_display.h"

struct _FlEngineDisplay {
  GObject parent_instance;
};

G_DEFINE_TYPE(FlEngineDisplay, fl_engine_display, G_TYPE_OBJECT)

static void fl_engine_display_class_init(FlEngineDisplayClass* klass) {}

static void fl_engine_display_init(FlEngineDisplay* self) {}

FlEngineDisplay* fl_engine_display_new() {
  FlEngineDisplay* self =
      FL_ENGINE_DISPLAY(g_object_new(fl_engine_display_get_type(), nullptr));
  return self;
}
