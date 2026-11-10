// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_VIEW_ACCESSIBLE_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_VIEW_ACCESSIBLE_H_

#if !defined(__FLUTTER_LINUX_INSIDE__) && !defined(FLUTTER_LINUX_COMPILATION)
#error "Only <flutter_linux/flutter_linux.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/linux/fl_accessible_node.h"
#include "flutter/shell/platform/linux/fl_atspi_application.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_engine.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlViewAccessible,
                     fl_view_accessible,
                     FL,
                     VIEW_ACCESSIBLE,
                     GObject)

/**
 * FlViewAccessible:
 *
 * #FlViewAccessible is an object that exposes accessibility information for an
 * #FlView to assistive technologies using AT-SPI. It implements
 * #FlAtspiAccessible, and contains the #FlAccessibleNode objects for the
 * semantics tree.
 */

/**
 * fl_view_accessible_new:
 * @engine: the #FlEngine.
 * @application: the #FlAtspiApplication to expose the view with.
 * @view_id: the Flutter view id.
 *
 * Creates a new accessibility object that exposes Flutter accessibility
 * information over AT-SPI.
 *
 * Returns: a new #FlViewAccessible.
 */
FlViewAccessible* fl_view_accessible_new(FlEngine* engine,
                                         FlAtspiApplication* application,
                                         FlutterViewId view_id);

/**
 * fl_view_accessible_embed:
 * @accessible: an #FlViewAccessible.
 * @widget: the widget showing the view, with an #FlSocketAccessible.
 *
 * Embeds the accessibility tree into the accessible of @widget.
 */
void fl_view_accessible_embed(FlViewAccessible* accessible, GtkWidget* widget);

/**
 * fl_view_accessible_handle_update_semantics:
 * @accessible: an #FlViewAccessible.
 * @update: semantic update information.
 *
 * Handle a semantics update from Flutter.
 */
void fl_view_accessible_handle_update_semantics(
    FlViewAccessible* accessible,
    const FlutterSemanticsUpdate2* update);

/**
 * fl_view_accessible_send_announcement:
 * @accessible: an #FlViewAccessible.
 * @message: text to be announced.
 * @assertive: %TRUE if the message should be in an assertive voice.
 *
 * Sends an annoucement to a screen reader.
 */
void fl_view_accessible_send_announcement(FlViewAccessible* accessible,
                                          const char* message,
                                          gboolean assertive);

/**
 * fl_view_accessible_lookup_node:
 * @accessible: an #FlViewAccessible.
 * @id: a semantics node ID.
 *
 * Returns: (transfer none): the node with @id or %NULL.
 */
FlAccessibleNode* fl_view_accessible_lookup_node(FlViewAccessible* accessible,
                                                 int32_t id);

/**
 * fl_view_accessible_get_custom_action:
 * @accessible: an #FlViewAccessible.
 * @id: a custom action ID.
 * @label: (out): location to write the label of the action.
 * @override_action: (out): location to write the standard action this action
 * describes, or 0.
 *
 * Returns: %TRUE if the custom action is known.
 */
gboolean fl_view_accessible_get_custom_action(
    FlViewAccessible* accessible,
    int32_t id,
    const gchar** label,
    FlutterSemanticsAction* override_action);

/**
 * fl_view_accessible_get_pixel_ratio:
 * @accessible: an #FlViewAccessible.
 *
 * Returns: the number of semantics units (physical pixels) per logical pixel.
 */
gdouble fl_view_accessible_get_pixel_ratio(FlViewAccessible* accessible);

/**
 * fl_view_accessible_dispatch_action:
 * @accessible: an #FlViewAccessible.
 * @node_id: the semantics node ID.
 * @action: the action to perform.
 * @data: (allow-none): the action data.
 *
 * Sends a semantics action to Flutter.
 */
void fl_view_accessible_dispatch_action(FlViewAccessible* accessible,
                                        int32_t node_id,
                                        FlutterSemanticsAction action,
                                        GBytes* data);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_VIEW_ACCESSIBLE_H_
