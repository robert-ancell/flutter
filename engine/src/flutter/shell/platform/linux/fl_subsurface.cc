#include <epoxy/egl.h>
#include <gdk/gdkwayland.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

#include "fl_subsurface.h"

struct _FlSubsurface {
  GtkWidget parent_instance;

  FlWaylandDisplay* display;

  FlOpenGLManager* opengl_manager;

  struct wl_surface* surface;
  struct wl_subsurface* subsurface;
  struct wl_egl_window* egl_window;
  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLContext egl_shared_context;
  EGLSurface egl_surface;
};

enum { SIGNAL_RESIZE, LAST_SIGNAL };

static guint fl_subsurface_signals[LAST_SIGNAL];

G_DEFINE_TYPE(FlSubsurface, fl_subsurface, GTK_TYPE_WIDGET);

static void fl_subsurface_dispose(GObject* object) {
  FlSubsurface* self = FL_SUBSURFACE(object);

  g_clear_object(&self->display);
  g_clear_object(&self->opengl_manager);

  G_OBJECT_CLASS(fl_subsurface_parent_class)->dispose(object);
}

static void fl_subsurface_realize(GtkWidget* widget) {
  FlSubsurface* self = FL_SUBSURFACE(widget);

  GTK_WIDGET_CLASS(fl_subsurface_parent_class)->realize(widget);

  GdkWindow* parent_window = gtk_widget_get_parent_window(widget);
  struct wl_surface* parent_surface =
      gdk_wayland_window_get_wl_surface(parent_window);

  struct wl_compositor* compositor = gdk_wayland_display_get_wl_compositor(
      fl_wayland_display_get_display(self->display));
  self->surface = wl_compositor_create_surface(compositor);
  self->subsurface = wl_subcompositor_get_subsurface(
      fl_wayland_display_get_wl_subcompositor(self->display), self->surface,
      parent_surface);

  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  self->egl_window =
      wl_egl_window_create(self->surface, allocation.width, allocation.height);
  self->egl_display = fl_opengl_manager_get_display(self->opengl_manager);

  eglBindAPI(EGL_OPENGL_API);
  eglInitialize(self->egl_display, NULL, NULL);
  EGLConfig egl_config = fl_opengl_manager_get_config(self->opengl_manager);
  self->egl_context =
      eglCreateContext(self->egl_display, egl_config, EGL_NO_CONTEXT, NULL);
  self->egl_shared_context =
      eglCreateContext(self->egl_display, egl_config, self->egl_context, NULL);
  self->egl_surface = fl_opengl_manager_create_window_surface(
      self->opengl_manager,
      reinterpret_cast<EGLNativeWindowType>(self->egl_window));
}

static void fl_subsurface_unrealize(GtkWidget* widget) {
  // FlSubsurface *self = FL_SUBSURFACE(widget);

  // FIXME: Destroy surface, subsurface, egl_window

  GTK_WIDGET_CLASS(fl_subsurface_parent_class)->unrealize(widget);
}

static void fl_subsurface_size_allocate(GtkWidget* widget,
                                        GtkAllocation* allocation) {
  FlSubsurface* self = FL_SUBSURFACE(widget);

  GTK_WIDGET_CLASS(fl_subsurface_parent_class)
      ->size_allocate(widget, allocation);

  // FIXME: The 26,70 offset is the allocation of the GtkEventBox parent, need
  // to work out how to get this.
  wl_subsurface_set_position(self->subsurface, 26,
                             70);  // allocation->x, allocation->y);
  wl_egl_window_resize(self->egl_window, allocation->width, allocation->height,
                       0, 0);

  g_signal_emit(self, fl_subsurface_signals[SIGNAL_RESIZE], 0);
}

static void fl_subsurface_class_init(FlSubsurfaceClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_subsurface_dispose;
  GTK_WIDGET_CLASS(klass)->realize = fl_subsurface_realize;
  GTK_WIDGET_CLASS(klass)->unrealize = fl_subsurface_unrealize;
  GTK_WIDGET_CLASS(klass)->size_allocate = fl_subsurface_size_allocate;

  // FIXME: Dimensions
  fl_subsurface_signals[SIGNAL_RESIZE] =
      g_signal_new("resize", fl_subsurface_get_type(), G_SIGNAL_RUN_LAST, 0,
                   NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void fl_subsurface_init(FlSubsurface* self) {
  gtk_widget_set_has_window(GTK_WIDGET(self), FALSE);
}

FlSubsurface* fl_subsurface_new(FlWaylandDisplay* display,
                                FlOpenGLManager* opengl_manager) {
  FlSubsurface* self =
      FL_SUBSURFACE(g_object_new(fl_subsurface_get_type(), NULL));

  self->display = FL_WAYLAND_DISPLAY(g_object_ref(display));
  self->opengl_manager = FL_OPENGL_MANAGER(g_object_ref(opengl_manager));

  return self;
}

void fl_subsurface_make_current(FlSubsurface* self) {
  g_return_if_fail(FL_IS_SUBSURFACE(self));

  eglMakeCurrent(self->egl_display, self->egl_surface, self->egl_surface,
                 self->egl_context);
}

void fl_subsurface_swap_buffers(FlSubsurface* self) {
  g_return_if_fail(FL_IS_SUBSURFACE(self));
  fl_opengl_manager_swap_buffers(self->opengl_manager, self->egl_surface);
}
