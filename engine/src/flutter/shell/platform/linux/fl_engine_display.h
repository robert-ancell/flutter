// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ENGINE_DISPLAY_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ENGINE_DISPLAY_H_

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlEngineDisplay,
                     fl_engine_display,
                     FL,
                     ENGINE_DISPLAY,
                     GObject);

/**
 * fl_engine_display_new:
 *
 * Creates a new object to represent display information.
 *
 * Returns: a new #FlEngineDisplay.
 */
FlEngineDisplay* fl_engine_display_new();

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ENGINE_DISPLAY_H_
