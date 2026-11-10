// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_PRIVATE_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_PRIVATE_H_

#include "flutter/shell/platform/linux/fl_atspi_application.h"

G_BEGIN_DECLS

/**
 * fl_atspi_application_make_reference:
 * @application: an #FlAtspiApplication.
 * @accessible: (allow-none): an #FlAtspiAccessible in @application or %NULL.
 *
 * Makes an AT-SPI reference to @accessible, exporting it if it hasn't been
 * already. Must only be called when connected to the accessibility bus.
 *
 * Returns: (transfer floating): a reference, of type (so).
 */
GVariant* fl_atspi_application_make_reference(FlAtspiApplication* application,
                                              FlAtspiAccessible* accessible);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_PRIVATE_H_
