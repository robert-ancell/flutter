// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_SUBSURFACE_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_SUBSURFACE_H_

#include <gtk/gtk.h>

#include "fl_opengl_manager.h"
#include "fl_wayland_display.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlSubsurface, fl_subsurface, FL, SUBSURFACE, GtkWidget);

FlSubsurface* fl_subsurface_new(FlWaylandDisplay* display,
                                FlOpenGLManager* opengl_manager);

void fl_subsurface_make_current(FlSubsurface* subsurface);

void fl_subsurface_swap_buffers(FlSubsurface* subsurface);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_SUBSURFACE_H_
