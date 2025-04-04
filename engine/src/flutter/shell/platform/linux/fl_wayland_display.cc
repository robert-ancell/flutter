#include <gdk/gdkwayland.h>

#include "fl_wayland_display.h"

struct _FlWaylandDisplay {
  GObject parent_instance;

  GdkDisplay* display;
  struct wl_subcompositor* subcompositor;
};

G_DEFINE_TYPE(FlWaylandDisplay, fl_wayland_display, G_TYPE_OBJECT);

static void fl_wayland_display_dispose(GObject* object) {
  FlWaylandDisplay* self = FL_WAYLAND_DISPLAY(object);

  g_clear_object(&self->display);
  // FIXME: subcompositor

  G_OBJECT_CLASS(fl_wayland_display_parent_class)->dispose(object);
}

static void fl_wayland_display_class_init(FlWaylandDisplayClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_wayland_display_dispose;
}

static void fl_wayland_display_init(FlWaylandDisplay* self) {}

static void registry_add_object(void* data,
                                struct wl_registry* registry,
                                uint32_t name,
                                const char* interface,
                                uint32_t version) {
  FlWaylandDisplay* self = FL_WAYLAND_DISPLAY(data);

  if (!strcmp(interface, wl_subcompositor_interface.name)) {
    self->subcompositor = reinterpret_cast<wl_subcompositor*>(
        wl_registry_bind(registry, name, &wl_subcompositor_interface, 1));
  }
}

static void registry_remove_object(void* data,
                                   struct wl_registry* registry,
                                   uint32_t name) {}

static struct wl_registry_listener registry_listener = {
    &registry_add_object, &registry_remove_object};

FlWaylandDisplay* fl_wayland_display_open(GdkDisplay* display) {
  FlWaylandDisplay* self =
      FL_WAYLAND_DISPLAY(g_object_new(fl_wayland_display_get_type(), NULL));

  self->display = GDK_DISPLAY(g_object_ref(display));

  struct wl_display* d =
      gdk_wayland_display_get_wl_display(GDK_WAYLAND_DISPLAY(display));
  wl_registry_add_listener(wl_display_get_registry(d), &registry_listener,
                           self);
  wl_display_roundtrip(d);

  return self;
}

GdkDisplay* fl_wayland_display_get_display(FlWaylandDisplay* self) {
  g_return_val_if_fail(FL_IS_WAYLAND_DISPLAY, NULL);
  return self->display;
}

struct wl_subcompositor* fl_wayland_display_get_wl_subcompositor(
    FlWaylandDisplay* self) {
  g_return_val_if_fail(FL_IS_WAYLAND_DISPLAY, NULL);
  return self->subcompositor;
}
