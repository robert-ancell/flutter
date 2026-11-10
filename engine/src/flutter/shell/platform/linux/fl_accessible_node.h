// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_NODE_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_NODE_H_

#include <glib-object.h>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/linux/fl_atspi_accessible.h"

G_BEGIN_DECLS

typedef struct _FlViewAccessible FlViewAccessible;

G_DECLARE_FINAL_TYPE(FlAccessibleNode,
                     fl_accessible_node,
                     FL,
                     ACCESSIBLE_NODE,
                     GObject);

/**
 * FlAccessibleNode:
 *
 * #FlAccessibleNode is an object that exposes a Flutter semantics node to
 * assistive technologies over AT-SPI.
 */

/**
 * fl_accessible_node_new:
 * @view: the #FlViewAccessible this node is in.
 * @id: the semantics node ID.
 *
 * Creates a new accessibility object for a Flutter semantics node.
 *
 * Returns: a new #FlAccessibleNode.
 */
FlAccessibleNode* fl_accessible_node_new(FlViewAccessible* view, int32_t id);

/**
 * fl_accessible_node_get_id:
 * @node: an #FlAccessibleNode.
 *
 * Returns: the semantics node ID.
 */
int32_t fl_accessible_node_get_id(FlAccessibleNode* node);

/**
 * fl_accessible_node_update:
 * @node: an #FlAccessibleNode.
 * @semantics: the new properties of the node.
 *
 * Replaces the properties of the node with those in @semantics.
 */
void fl_accessible_node_update(FlAccessibleNode* node,
                               const FlutterSemanticsNode2* semantics);

/**
 * fl_accessible_node_get_child_ids:
 * @node: an #FlAccessibleNode.
 *
 * Returns: (transfer none) (element-type gint32): the IDs of the children in
 * traversal order.
 */
GArray* fl_accessible_node_get_child_ids(FlAccessibleNode* node);

/**
 * fl_accessible_node_get_parent_node:
 * @node: an #FlAccessibleNode.
 *
 * Returns: (transfer none): the parent node or %NULL.
 */
FlAccessibleNode* fl_accessible_node_get_parent_node(FlAccessibleNode* node);

/**
 * fl_accessible_node_set_parent_node:
 * @node: an #FlAccessibleNode.
 * @parent: (allow-none): the parent node.
 *
 * Sets the parent of this node.
 */
void fl_accessible_node_set_parent_node(FlAccessibleNode* node,
                                        FlAccessibleNode* parent);

/**
 * fl_accessible_node_is_focused:
 * @node: an #FlAccessibleNode.
 *
 * Returns: %TRUE if Flutter reports this node as focused.
 */
gboolean fl_accessible_node_is_focused(FlAccessibleNode* node);

/**
 * fl_accessible_node_is_live_region:
 * @node: an #FlAccessibleNode.
 *
 * Returns: %TRUE if changes to the name of this node should be announced.
 */
gboolean fl_accessible_node_is_live_region(FlAccessibleNode* node);

/**
 * fl_accessible_node_continue_set_value:
 * @node: an #FlAccessibleNode.
 * @old_value: (allow-none): the value before the update.
 *
 * Steps the value towards the value being set, after an update.
 */
void fl_accessible_node_continue_set_value(FlAccessibleNode* node,
                                           FlAtspiValue* old_value);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_NODE_H_
