#include <gtk/gtk.h>

#include "fl_wayland_display.h"

#pragma once

G_DECLARE_FINAL_TYPE(FlSubsurface, fl_subsurface, FL, SUBSURFACE, GtkWidget);

FlSubsurface* fl_subsurface_new(FlWaylandDisplay* display);

void fl_subsurface_make_current(FlSubsurface* subsurface);

void fl_subsurface_swap_buffers(FlSubsurface* subsurface);
