// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_OPENGL_MANAGER_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_OPENGL_MANAGER_H_

#include <epoxy/egl.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlOpenGLManager,
                     fl_opengl_manager,
                     FL,
                     OPENGL_MANAGER,
                     GObject)

/**
 * fl_opengl_manager_new:
 *
 * Creates an object that allows Flutter to render by OpenGL ES.
 *
 * Returns: a new #FlOpenGLManager.
 */
FlOpenGLManager* fl_opengl_manager_new();

/**
 * fl_opengl_manager_create_contexts:
 * @manager: an #FlOpenGLManager.
 * @window: the window that is being rendered on.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL to ignore.
 *
 * Create rendering contexts.
 *
 * Returns: %TRUE if contexts were created, %FALSE if there was an error.
 */
gboolean fl_opengl_manager_create_contexts(FlOpenGLManager* manager,
                                           GError** error);

/**
 * fl_opengl_manager_make_current:
 * @manager: an #FlOpenGLManager.
 * @surface: surface to render to.
 *
 * Makes the rendering context current.
 */
void fl_opengl_manager_make_current(FlOpenGLManager* manager,
                                    EGLSurface surface);

/**
 * fl_opengl_manager_make_resource_current:
 * @manager: an #FlOpenGLManager.
 *
 * Makes the resource rendering context current.
 */
void fl_opengl_manager_make_resource_current(FlOpenGLManager* manager);

/**
 * fl_opengl_manager_clear_current:
 * @manager: an #FlOpenGLManager.
 *
 * Clears the current rendering context.
 */
void fl_opengl_manager_clear_current(FlOpenGLManager* manager);

// FIXME
EGLSurface fl_opengl_manager_create_window_surface(FlOpenGLManager* manager,
                                                   EGLNativeWindowType window);

// FIXME
void fl_opengl_manager_swap_buffers(FlOpenGLManager* manager,
                                    EGLSurface surface);

EGLDisplay fl_opengl_manager_get_display(FlOpenGLManager* manager);

EGLConfig fl_opengl_manager_get_config(FlOpenGLManager* manager);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_OPENGL_MANAGER_H_
