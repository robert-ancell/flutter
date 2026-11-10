// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_H_

#include <atk/atk.h>
#include <gio/gio.h>

#include "flutter/shell/platform/linux/fl_atspi_accessible.h"

G_BEGIN_DECLS

/**
 * FlAtspiProperty:
 * @FL_ATSPI_PROPERTY_NAME: the name.
 * @FL_ATSPI_PROPERTY_DESCRIPTION: the description.
 * @FL_ATSPI_PROPERTY_ROLE: the role.
 * @FL_ATSPI_PROPERTY_VALUE: the value of a range.
 *
 * Properties of an #FlAtspiAccessible that change events are reported for.
 */
typedef enum {
  FL_ATSPI_PROPERTY_NAME,
  FL_ATSPI_PROPERTY_DESCRIPTION,
  FL_ATSPI_PROPERTY_ROLE,
  FL_ATSPI_PROPERTY_VALUE,
} FlAtspiProperty;

G_DECLARE_FINAL_TYPE(FlAtspiApplication,
                     fl_atspi_application,
                     FL,
                     ATSPI_APPLICATION,
                     GObject);

/**
 * FlAtspiApplication:
 *
 * #FlAtspiApplication exposes accessible windows to assistive technologies,
 * such as screen readers, using AT-SPI. It is the root #FlAtspiAccessible,
 * whose children are the windows.
 */

/**
 * fl_atspi_application_new:
 *
 * Creates a new application that exposes windows over AT-SPI. It connects to
 * the accessibility bus when fl_atspi_application_start() is called.
 *
 * Returns: a new #FlAtspiApplication.
 */
FlAtspiApplication* fl_atspi_application_new();

/**
 * fl_atspi_application_new_with_connection:
 * @connection: a connection to use as the accessibility bus, e.g. for testing.
 *
 * Creates a new application that exposes windows over AT-SPI on @connection.
 *
 * Returns: a new #FlAtspiApplication.
 */
FlAtspiApplication* fl_atspi_application_new_with_connection(
    GDBusConnection* connection);

/**
 * fl_atspi_application_start:
 * @application: an #FlAtspiApplication.
 *
 * Connects to the accessibility bus. Does nothing if already connected or
 * connecting.
 */
void fl_atspi_application_start(FlAtspiApplication* application);

/**
 * fl_atspi_application_add_window:
 * @application: an #FlAtspiApplication.
 * @window: the window object, which must be removed before it is destroyed.
 *
 * Adds a window to the application's children.
 */
void fl_atspi_application_add_window(FlAtspiApplication* application,
                                     FlAtspiAccessible* window);

/**
 * fl_atspi_application_remove_window:
 * @application: an #FlAtspiApplication.
 * @window: a window added with fl_atspi_application_add_window().
 *
 * Removes a window from the application's children.
 */
void fl_atspi_application_remove_window(FlAtspiApplication* application,
                                        FlAtspiAccessible* window);

/**
 * fl_atspi_application_set_window_socket:
 * @application: an #FlAtspiApplication.
 * @window: a window added with fl_atspi_application_add_window().
 * @socket: the socket in the GTK accessibility tree to embed @window into.
 *
 * Sets the socket to embed @window into when connected to the accessibility
 * bus.
 */
void fl_atspi_application_set_window_socket(FlAtspiApplication* application,
                                            FlAtspiAccessible* window,
                                            AtkSocket* socket);

/**
 * fl_atspi_application_emit_children_changed:
 * @application: an #FlAtspiApplication.
 * @parent: the object whose children changed.
 * @added: %TRUE if @child was added, %FALSE if it was removed.
 * @index: the index of @child in @parent.
 * @child: the child that was added or removed.
 *
 * Reports a child being added or removed.
 */
void fl_atspi_application_emit_children_changed(FlAtspiApplication* application,
                                                FlAtspiAccessible* parent,
                                                gboolean added,
                                                gint index,
                                                FlAtspiAccessible* child);

/**
 * fl_atspi_application_emit_property_changed:
 * @application: an #FlAtspiApplication.
 * @source: the object that changed.
 * @property: the property that changed.
 *
 * Reports a property of @source changing to its current value.
 */
void fl_atspi_application_emit_property_changed(FlAtspiApplication* application,
                                                FlAtspiAccessible* source,
                                                FlAtspiProperty property);

/**
 * fl_atspi_application_emit_state_changed:
 * @application: an #FlAtspiApplication.
 * @source: the object the state changed on.
 * @state: the state that changed.
 * @value: the new value of @state.
 *
 * Reports a state of @source changing.
 */
void fl_atspi_application_emit_state_changed(FlAtspiApplication* application,
                                             FlAtspiAccessible* source,
                                             FlAtspiState state,
                                             gboolean value);

/**
 * fl_atspi_application_emit_text_changed:
 * @application: an #FlAtspiApplication.
 * @source: the object the text changed in.
 * @inserted: %TRUE if @text was inserted, %FALSE if it was deleted.
 * @offset: the character offset of the change.
 * @text: the text inserted or deleted.
 *
 * Reports text being inserted or deleted.
 */
void fl_atspi_application_emit_text_changed(FlAtspiApplication* application,
                                            FlAtspiAccessible* source,
                                            gboolean inserted,
                                            gint offset,
                                            const gchar* text);

/**
 * fl_atspi_application_emit_text_caret_moved:
 * @application: an #FlAtspiApplication.
 * @source: the object the caret moved in.
 * @offset: the new character offset of the caret.
 *
 * Reports the text caret moving.
 */
void fl_atspi_application_emit_text_caret_moved(FlAtspiApplication* application,
                                                FlAtspiAccessible* source,
                                                gint offset);

/**
 * fl_atspi_application_emit_text_selection_changed:
 * @application: an #FlAtspiApplication.
 * @source: the object the text selection changed in.
 *
 * Reports the text selection changing.
 */
void fl_atspi_application_emit_text_selection_changed(
    FlAtspiApplication* application,
    FlAtspiAccessible* source);

/**
 * fl_atspi_application_emit_selection_changed:
 * @application: an #FlAtspiApplication.
 * @source: the object whose selected children changed.
 *
 * Reports the selected children changing.
 */
void fl_atspi_application_emit_selection_changed(
    FlAtspiApplication* application,
    FlAtspiAccessible* source);

/**
 * fl_atspi_application_emit_announcement:
 * @application: an #FlAtspiApplication.
 * @source: the object making the announcement.
 * @message: the text to announce.
 * @assertive: %TRUE if the message should interrupt the screen reader.
 *
 * Asks screen readers to announce a message.
 */
void fl_atspi_application_emit_announcement(FlAtspiApplication* application,
                                            FlAtspiAccessible* source,
                                            const gchar* message,
                                            gboolean assertive);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_APPLICATION_H_
