// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_OBJECT_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_OBJECT_H_

#include <gio/gio.h>

#include "flutter/shell/platform/linux/fl_atspi_accessible.h"
#include "flutter/shell/platform/linux/fl_atspi_application.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlAtspiObject, fl_atspi_object, FL, ATSPI_OBJECT, GObject);

/**
 * FlAtspiObject:
 *
 * #FlAtspiObject exports an #FlAtspiAccessible on the accessibility bus.
 */

/**
 * fl_atspi_object_new:
 * @application: the application @accessible is in.
 * @accessible: the accessible to export. It is not referenced, so the
 * #FlAtspiObject must be destroyed before it.
 * @connection: the connection to the accessibility bus.
 * @path: the object path to export @accessible at.
 *
 * Creates a new object that exports @accessible at @path.
 *
 * Returns: a new #FlAtspiObject.
 */
FlAtspiObject* fl_atspi_object_new(FlAtspiApplication* application,
                                   FlAtspiAccessible* accessible,
                                   GDBusConnection* connection,
                                   const gchar* path);

/**
 * fl_atspi_object_get_path:
 * @object: an #FlAtspiObject.
 *
 * Gets the object path @object is exported at.
 *
 * Returns: an object path.
 */
const gchar* fl_atspi_object_get_path(FlAtspiObject* object);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_OBJECT_H_
