// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_accessible.h"

G_DEFINE_INTERFACE(FlAtspiAccessible, fl_atspi_accessible, G_TYPE_OBJECT)

static void fl_atspi_accessible_default_init(
    FlAtspiAccessibleInterface* iface) {}

#define GET_IFACE(accessible) FL_ATSPI_ACCESSIBLE_GET_IFACE(accessible)

FlAtspiAction* fl_atspi_action_new(const gchar* name,
                                   const gchar* description) {
  FlAtspiAction* action = g_new0(FlAtspiAction, 1);
  action->name = g_strdup(name);
  action->description = g_strdup(description);
  return action;
}

void fl_atspi_action_free(FlAtspiAction* action) {
  g_free(action->name);
  g_free(action->description);
  g_free(action);
}

FlAtspiRelation* fl_atspi_relation_new(FlAtspiRelationType type) {
  FlAtspiRelation* relation = g_new0(FlAtspiRelation, 1);
  relation->type = type;
  relation->targets = g_ptr_array_new_with_free_func(g_object_unref);
  return relation;
}

void fl_atspi_relation_free(FlAtspiRelation* relation) {
  g_ptr_array_unref(relation->targets);
  g_free(relation);
}

FlAtspiValue* fl_atspi_value_new(gdouble minimum,
                                 gdouble maximum,
                                 gdouble increment,
                                 gdouble current,
                                 const gchar* text) {
  FlAtspiValue* value = g_new0(FlAtspiValue, 1);
  value->minimum = minimum;
  value->maximum = maximum;
  value->increment = increment;
  value->current = current;
  value->text = g_strdup(text);
  return value;
}

void fl_atspi_value_free(FlAtspiValue* value) {
  g_free(value->text);
  g_free(value);
}

FlAtspiAccessible* fl_atspi_accessible_get_parent(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_IFACE(self)->get_parent(self);
}

gsize fl_atspi_accessible_get_n_children(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), 0);
  return GET_IFACE(self)->get_n_children(self);
}

FlAtspiAccessible* fl_atspi_accessible_get_child(FlAtspiAccessible* self,
                                                 gsize index) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_IFACE(self)->get_child(self, index);
}

gint fl_atspi_accessible_get_index_in_parent(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), -1);
  FlAtspiAccessible* parent = fl_atspi_accessible_get_parent(self);
  if (parent == nullptr) {
    return -1;
  }
  gsize n_children = fl_atspi_accessible_get_n_children(parent);
  for (gsize i = 0; i < n_children; i++) {
    if (fl_atspi_accessible_get_child(parent, i) == self) {
      return i;
    }
  }
  return -1;
}

FlAtspiRole fl_atspi_accessible_get_role(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FL_ATSPI_ROLE_INVALID);
  return GET_IFACE(self)->get_role(self);
}

// Calls an optional string getter, returning "" if not implemented.
#define GET_STRING(self, method)                                      \
  (GET_IFACE(self)->method != nullptr ? GET_IFACE(self)->method(self) \
                                      : g_strdup(""))

gchar* fl_atspi_accessible_get_name(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_STRING(self, get_name);
}

gchar* fl_atspi_accessible_get_description(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_STRING(self, get_description);
}

gchar* fl_atspi_accessible_get_help_text(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_STRING(self, get_help_text);
}

gchar* fl_atspi_accessible_get_accessible_id(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_STRING(self, get_accessible_id);
}

guint64 fl_atspi_accessible_get_states(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), 0);
  return GET_IFACE(self)->get_states(self);
}

GHashTable* fl_atspi_accessible_get_attributes(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  if (GET_IFACE(self)->get_attributes != nullptr) {
    return GET_IFACE(self)->get_attributes(self);
  }
  return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

GPtrArray* fl_atspi_accessible_get_relations(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  if (GET_IFACE(self)->get_relations != nullptr) {
    return GET_IFACE(self)->get_relations(self);
  }
  return g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(fl_atspi_relation_free));
}

gboolean fl_atspi_accessible_get_extents(FlAtspiAccessible* self,
                                         GdkRectangle* extents) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  *extents = {};
  return GET_IFACE(self)->get_extents != nullptr &&
         GET_IFACE(self)->get_extents(self, extents);
}

void fl_atspi_accessible_get_origin(FlAtspiAccessible* self,
                                    FlAtspiCoordType coord_type,
                                    gint* x,
                                    gint* y) {
  g_return_if_fail(FL_IS_ATSPI_ACCESSIBLE(self));
  *x = *y = 0;
  if (GET_IFACE(self)->get_origin != nullptr) {
    GET_IFACE(self)->get_origin(self, coord_type, x, y);
  }
}

gboolean fl_atspi_accessible_grab_focus(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->grab_focus != nullptr &&
         GET_IFACE(self)->grab_focus(self);
}

gboolean fl_atspi_accessible_scroll_to(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->scroll_to != nullptr &&
         GET_IFACE(self)->scroll_to(self);
}

FlAtspiAccessible* fl_atspi_accessible_get_accessible_at_point(
    FlAtspiAccessible* self,
    gint x,
    gint y) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  GdkRectangle extents;
  if (!fl_atspi_accessible_get_extents(self, &extents) || x < extents.x ||
      y < extents.y || x >= extents.x + extents.width ||
      y >= extents.y + extents.height ||
      (fl_atspi_accessible_get_states(self) &
       FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_SHOWING)) == 0) {
    return nullptr;
  }
  // Later children are drawn on top, so are checked first.
  for (gsize i = fl_atspi_accessible_get_n_children(self); i > 0; i--) {
    FlAtspiAccessible* child = fl_atspi_accessible_get_child(self, i - 1);
    FlAtspiAccessible* result =
        child != nullptr
            ? fl_atspi_accessible_get_accessible_at_point(child, x, y)
            : nullptr;
    if (result != nullptr) {
      return result;
    }
  }
  return self;
}

GPtrArray* fl_atspi_accessible_get_actions(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  if (GET_IFACE(self)->get_actions != nullptr) {
    return GET_IFACE(self)->get_actions(self);
  }
  return g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(fl_atspi_action_free));
}

gboolean fl_atspi_accessible_do_action(FlAtspiAccessible* self, gsize index) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->do_action != nullptr &&
         GET_IFACE(self)->do_action(self, index);
}

FlAtspiValue* fl_atspi_accessible_get_value(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  if (GET_IFACE(self)->get_value == nullptr) {
    return nullptr;
  }
  return GET_IFACE(self)->get_value(self);
}

gboolean fl_atspi_accessible_set_value(FlAtspiAccessible* self, gdouble value) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->set_value != nullptr &&
         GET_IFACE(self)->set_value(self, value);
}

gboolean fl_atspi_accessible_has_text(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->has_text != nullptr &&
         GET_IFACE(self)->has_text(self);
}

gboolean fl_atspi_accessible_is_editable_text(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->is_editable_text != nullptr &&
         GET_IFACE(self)->is_editable_text(self);
}

gchar* fl_atspi_accessible_get_text(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  return GET_STRING(self, get_text);
}

gint fl_atspi_accessible_get_caret_offset(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), -1);
  return GET_IFACE(self)->get_caret_offset != nullptr
             ? GET_IFACE(self)->get_caret_offset(self)
             : -1;
}

gboolean fl_atspi_accessible_get_selection(FlAtspiAccessible* self,
                                           gint* start,
                                           gint* end) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->get_selection != nullptr &&
         GET_IFACE(self)->get_selection(self, start, end);
}

gboolean fl_atspi_accessible_set_selection(FlAtspiAccessible* self,
                                           gint start,
                                           gint end) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->set_selection != nullptr &&
         GET_IFACE(self)->set_selection(self, start, end);
}

gboolean fl_atspi_accessible_set_text_contents(FlAtspiAccessible* self,
                                               const gchar* text) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->set_text_contents != nullptr &&
         GET_IFACE(self)->set_text_contents(self, text);
}

gboolean fl_atspi_accessible_do_clipboard_action(
    FlAtspiAccessible* self,
    FlAtspiClipboardAction action) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->do_clipboard_action != nullptr &&
         GET_IFACE(self)->do_clipboard_action(self, action);
}

gboolean fl_atspi_accessible_is_selection_container(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->is_selection_container != nullptr &&
         GET_IFACE(self)->is_selection_container(self);
}

GArray* fl_atspi_accessible_get_selected_children(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), nullptr);
  if (GET_IFACE(self)->get_selected_children != nullptr) {
    return GET_IFACE(self)->get_selected_children(self);
  }
  return g_array_new(FALSE, FALSE, sizeof(guint));
}

gboolean fl_atspi_accessible_select_child(FlAtspiAccessible* self,
                                          gsize index) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->select_child != nullptr &&
         GET_IFACE(self)->select_child(self, index);
}

gboolean fl_atspi_accessible_is_image(FlAtspiAccessible* self) {
  g_return_val_if_fail(FL_IS_ATSPI_ACCESSIBLE(self), FALSE);
  return GET_IFACE(self)->is_image != nullptr &&
         GET_IFACE(self)->is_image(self);
}
