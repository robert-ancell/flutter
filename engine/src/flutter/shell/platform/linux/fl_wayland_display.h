// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_WAYLAND_DISPLAY_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_WAYLAND_DISPLAY_H_

#include <gdk/gdk.h>
#include <wayland-client-protocol.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlWaylandDisplay,
                     fl_wayland_display,
                     FL,
                     WAYLAND_DISPLAY,
                     GObject);

FlWaylandDisplay* fl_wayland_display_open(GdkDisplay* display);

GdkDisplay* fl_wayland_display_get_display(FlWaylandDisplay* self);

struct wl_subcompositor* fl_wayland_display_get_wl_subcompositor(
    FlWaylandDisplay* self);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_WAYLAND_DISPLAY_H_
