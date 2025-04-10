// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <epoxy/egl.h>
#include <gdk/gdkwayland.h>

#include "flutter/shell/platform/linux/fl_opengl_manager.h"

struct _FlOpenGLManager {
  GObject parent_instance;

  EGLDisplay egl_display;
  EGLConfig egl_config;
  EGLContext render_context;
  EGLContext resource_context;
};

G_DEFINE_TYPE(FlOpenGLManager, fl_opengl_manager, G_TYPE_OBJECT)

static void fl_opengl_manager_dispose(GObject* object) {
  // FlOpenGLManager* self = FL_OPENGL_MANAGER(object);

  G_OBJECT_CLASS(fl_opengl_manager_parent_class)->dispose(object);
}

static void fl_opengl_manager_class_init(FlOpenGLManagerClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_opengl_manager_dispose;
}

static void fl_opengl_manager_init(FlOpenGLManager* self) {}

FlOpenGLManager* fl_opengl_manager_new() {
  FlOpenGLManager* self =
      FL_OPENGL_MANAGER(g_object_new(fl_opengl_manager_get_type(), nullptr));
  return self;
}

gboolean fl_opengl_manager_create_contexts(FlOpenGLManager* self,
                                           GError** error) {
  g_printerr("fl_opengl_manager_create_contexts thread=%p\n", g_thread_self());

  g_return_val_if_fail(FL_IS_OPENGL_MANAGER(self), FALSE);

  struct wl_display* display =
      gdk_wayland_display_get_wl_display(gdk_display_get_default());
  self->egl_display =
      eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display));

  eglBindAPI(EGL_OPENGL_API);
  eglInitialize(self->egl_display, NULL, NULL);

  EGLint config_attributes[] = {EGL_RED_SIZE,  8, EGL_GREEN_SIZE, 8,
                                EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                                EGL_NONE};
  EGLint num_config;
  if (!eglChooseConfig(self->egl_display, config_attributes, &self->egl_config,
                       1, &num_config) ||
      num_config == 0) {
    // FIXME: g_set_error
    g_warning("Failed to choose EGL config");
    return FALSE;
  }

  EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  self->render_context = eglCreateContext(self->egl_display, self->egl_config,
                                          EGL_NO_CONTEXT, context_attributes);
  self->resource_context =
      eglCreateContext(self->egl_display, self->egl_config,
                       self->render_context, context_attributes);

  return TRUE;
}

void fl_opengl_manager_make_current(FlOpenGLManager* self, EGLSurface surface) {
  g_return_if_fail(FL_IS_OPENGL_MANAGER(self));

  g_printerr("fl_opengl_manager_make_current thread=%p surface=%p\n",
             g_thread_self(), surface);
  eglMakeCurrent(self->egl_display, surface, surface, self->render_context);
}

void fl_opengl_manager_make_resource_current(FlOpenGLManager* self) {
  g_return_if_fail(FL_IS_OPENGL_MANAGER(self));

  g_printerr("fl_opengl_manager_make_resource_current thread=%p\n",
             g_thread_self());
  eglMakeCurrent(self->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                 self->resource_context);
}

void fl_opengl_manager_clear_current(FlOpenGLManager* self) {
  g_return_if_fail(FL_IS_OPENGL_MANAGER(self));

  g_printerr("fl_opengl_manager_clear_current thread=%p\n", g_thread_self());
  eglMakeCurrent(self->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                 EGL_NO_CONTEXT);
}

EGLSurface fl_opengl_manager_create_window_surface(FlOpenGLManager* self,
                                                   EGLNativeWindowType window) {
  g_return_val_if_fail(FL_IS_OPENGL_MANAGER(self), EGL_NO_SURFACE);

  return eglCreateWindowSurface(self->egl_display, self->egl_config, window,
                                nullptr);
}

void fl_opengl_manager_swap_buffers(FlOpenGLManager* self, EGLSurface surface) {
  g_return_if_fail(FL_IS_OPENGL_MANAGER(self));

  g_printerr("fl_opengl_manager_swap_buffers thread=%p surface=%p\n",
             g_thread_self(), surface);
  eglSwapInterval(self->egl_display, 0);  // FIXME?
  eglSwapBuffers(self->egl_display, surface);
}

EGLDisplay fl_opengl_manager_get_display(FlOpenGLManager* self) {
  g_return_val_if_fail(FL_IS_OPENGL_MANAGER(self), EGL_NO_DISPLAY);
  return self->egl_display;
}

EGLDisplay fl_opengl_manager_get_config(FlOpenGLManager* self) {
  g_return_val_if_fail(FL_IS_OPENGL_MANAGER(self), nullptr);
  return self->egl_config;
}
