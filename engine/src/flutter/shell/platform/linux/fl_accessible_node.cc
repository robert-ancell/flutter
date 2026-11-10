// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_accessible_node.h"

#include <cmath>
#include <cstdlib>

#include "flutter/shell/platform/linux/fl_atspi_text.h"
#include "flutter/shell/platform/linux/fl_view_accessible.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_standard_message_codec.h"

// The most increase or decrease actions sent to set a value.
static constexpr gint kMaxValueSteps = 1000;

struct _FlAccessibleNode {
  GObject parent_instance;

  // The view this node is in. The view owns the node.
  FlViewAccessible* view;

  int32_t id;
  FlAccessibleNode* parent;

  FlutterSemanticsFlags flags;
  int64_t actions;
  gchar* label;
  gchar* hint;
  gchar* value;
  gchar* tooltip;
  gchar* identifier;
  // In UTF-16 code units.
  int32_t text_selection_base;
  int32_t text_selection_extent;
  FlutterRect rect;
  FlutterTransformation transform;
  GArray* children;
  GArray* custom_actions;
  int32_t heading_level;

  // The value being set by repeatedly increasing or decreasing it.
  gboolean has_target_value;
  gdouble target_value;
  gint remaining_steps;
};

// An action that can be performed on a node.
typedef struct {
  FlutterSemanticsAction action;
  const gchar* name;
  const gchar* description;
  // For custom actions.
  int32_t custom_action_id;
} NodeAction;

static void fl_accessible_node_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlAccessibleNode,
    fl_accessible_node,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(fl_atspi_accessible_get_type(),
                          fl_accessible_node_atspi_accessible_iface_init))

static gboolean has_action(FlAccessibleNode* self,
                           FlutterSemanticsAction action) {
  return (self->actions & action) != 0;
}

static FlAccessibleNode* get_child_node(FlAccessibleNode* self, gsize index) {
  if (index >= self->children->len) {
    return nullptr;
  }
  return fl_view_accessible_lookup_node(
      self->view, g_array_index(self->children, int32_t, index));
}

static void send_action(FlAccessibleNode* self,
                        FlutterSemanticsAction action,
                        FlValue* data) {
  g_autoptr(GBytes) message = nullptr;
  if (data != nullptr) {
    g_autoptr(FlStandardMessageCodec) codec = fl_standard_message_codec_new();
    message =
        fl_message_codec_encode_message(FL_MESSAGE_CODEC(codec), data, nullptr);
  }
  fl_view_accessible_dispatch_action(self->view, self->id, action, message);
}

// Whether this node is a route without its own name.
static gboolean is_unnamed_route(FlAccessibleNode* self) {
  return self->flags.scopes_route && !self->flags.names_route && self->id != 0;
}

static gchar* get_name(FlAccessibleNode* self);

// The name of the route this node scopes, from the first descendant that
// names it.
static gchar* get_route_name(FlAccessibleNode* self) {
  for (guint i = 0; i < self->children->len; i++) {
    FlAccessibleNode* child = get_child_node(self, i);
    if (child == nullptr) {
      continue;
    }
    if (child->flags.names_route && !child->flags.scopes_route) {
      g_autofree gchar* name = get_name(child);
      if (name[0] != '\0') {
        return static_cast<gchar*>(g_steal_pointer(&name));
      }
    }
    // Nested routes are named separately.
    if (child->flags.scopes_route) {
      continue;
    }
    g_autofree gchar* name = get_route_name(child);
    if (name[0] != '\0') {
      return static_cast<gchar*>(g_steal_pointer(&name));
    }
  }
  return g_strdup("");
}

static gchar* get_name(FlAccessibleNode* self) {
  if (self->label[0] != '\0') {
    return g_strdup(self->label);
  }
  if (is_unnamed_route(self)) {
    return get_route_name(self);
  }
  // Icon buttons are often only described by their tooltip.
  return g_strdup(self->tooltip);
}

static FlAtspiRole get_role(FlAccessibleNode* self) {
  const FlutterSemanticsFlags* flags = &self->flags;
  if (flags->is_text_field) {
    return flags->is_obscured ? FL_ATSPI_ROLE_PASSWORD_TEXT
                              : FL_ATSPI_ROLE_ENTRY;
  }
  if (flags->is_slider) {
    return FL_ATSPI_ROLE_SLIDER;
  }
  if (flags->is_link) {
    return FL_ATSPI_ROLE_LINK;
  }
  if (flags->is_header) {
    return FL_ATSPI_ROLE_HEADING;
  }
  if (flags->is_image) {
    return FL_ATSPI_ROLE_IMAGE;
  }
  if (flags->is_checked != kFlutterCheckStateNone) {
    return flags->is_in_mutually_exclusive_group ? FL_ATSPI_ROLE_RADIO_BUTTON
                                                 : FL_ATSPI_ROLE_CHECK_BOX;
  }
  if (flags->is_toggled != kFlutterTristateNone) {
    return FL_ATSPI_ROLE_SWITCH;
  }
  if (flags->is_button || flags->is_keyboard_key) {
    return FL_ATSPI_ROLE_BUTTON;
  }
  if (flags->scopes_route && flags->names_route) {
    return FL_ATSPI_ROLE_DIALOG;
  }
  if (flags->has_implicit_scrolling) {
    return FL_ATSPI_ROLE_SCROLL_PANE;
  }
  if (self->children->len == 0 && self->label[0] != '\0') {
    return FL_ATSPI_ROLE_LABEL;
  }
  return FL_ATSPI_ROLE_PANEL;
}

// Parses a numeric value such as "50%" or "3.5". Returns FALSE if |value|
// isn't one.
static gboolean parse_number(const gchar* value,
                             gdouble* number,
                             gboolean* percent) {
  if (value[0] == '\0') {
    return FALSE;
  }
  gchar* end = nullptr;
  *number = g_ascii_strtod(value, &end);
  if (end == value) {
    return FALSE;
  }
  while (*end == ' ') {
    end++;
  }
  *percent = *end == '%';
  if (*percent) {
    end++;
  }
  return *end == '\0' && std::isfinite(*number);
}

// Applies |transform| to the point (x, y).
static void transform_point(const FlutterTransformation* transform,
                            gdouble* x,
                            gdouble* y) {
  gdouble tx =
      transform->scaleX * *x + transform->skewX * *y + transform->transX;
  gdouble ty =
      transform->skewY * *x + transform->scaleY * *y + transform->transY;
  gdouble w = transform->pers0 * *x + transform->pers1 * *y + transform->pers2;
  if (w != 0 && w != 1) {
    tx /= w;
    ty /= w;
  }
  *x = tx;
  *y = ty;
}

// Returns the standard and custom actions of this node.
static GArray* get_action_list(FlAccessibleNode* self) {
  static const struct {
    FlutterSemanticsAction action;
    const gchar* name;
  } kActions[] = {
      {kFlutterSemanticsActionTap, "click"},
      {kFlutterSemanticsActionLongPress, "long-press"},
      {kFlutterSemanticsActionIncrease, "increase"},
      {kFlutterSemanticsActionDecrease, "decrease"},
      {kFlutterSemanticsActionExpand, "expand"},
      {kFlutterSemanticsActionCollapse, "collapse"},
      {kFlutterSemanticsActionDismiss, "dismiss"},
      {kFlutterSemanticsActionScrollUp, "scroll-up"},
      {kFlutterSemanticsActionScrollDown, "scroll-down"},
      {kFlutterSemanticsActionScrollLeft, "scroll-left"},
      {kFlutterSemanticsActionScrollRight, "scroll-right"},
      {kFlutterSemanticsActionCopy, "copy"},
      {kFlutterSemanticsActionCut, "cut"},
      {kFlutterSemanticsActionPaste, "paste"},
  };

  GArray* actions = g_array_new(FALSE, TRUE, sizeof(NodeAction));
  for (const auto& entry : kActions) {
    if (has_action(self, entry.action)) {
      NodeAction action = {entry.action, entry.name, "", 0};
      g_array_append_val(actions, action);
    }
  }
  for (guint i = 0; i < self->custom_actions->len; i++) {
    int32_t id = g_array_index(self->custom_actions, int32_t, i);
    const gchar* label;
    FlutterSemanticsAction override_action;
    if (!fl_view_accessible_get_custom_action(self->view, id, &label,
                                              &override_action)) {
      continue;
    }
    if (override_action != 0) {
      // Describes a standard action, e.g. a long press that "Pins" an item.
      for (guint j = 0; j < actions->len; j++) {
        NodeAction* action = &g_array_index(actions, NodeAction, j);
        if (action->action == override_action) {
          action->description = label;
        }
      }
      continue;
    }
    NodeAction action = {kFlutterSemanticsActionCustomAction, label, label, id};
    g_array_append_val(actions, action);
  }
  return actions;
}

static FlAtspiAccessible* fl_accessible_node_get_parent(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (self->parent != nullptr) {
    return FL_ATSPI_ACCESSIBLE(self->parent);
  }
  return self->id == 0 ? FL_ATSPI_ACCESSIBLE(self->view) : nullptr;
}

static gsize fl_accessible_node_get_n_children(FlAtspiAccessible* accessible) {
  return FL_ACCESSIBLE_NODE(accessible)->children->len;
}

static FlAtspiAccessible* fl_accessible_node_get_child(
    FlAtspiAccessible* accessible,
    gsize index) {
  FlAccessibleNode* child =
      get_child_node(FL_ACCESSIBLE_NODE(accessible), index);
  return child != nullptr ? FL_ATSPI_ACCESSIBLE(child) : nullptr;
}

static FlAtspiRole fl_accessible_node_get_role(FlAtspiAccessible* accessible) {
  return get_role(FL_ACCESSIBLE_NODE(accessible));
}

static gchar* fl_accessible_node_get_name(FlAtspiAccessible* accessible) {
  return get_name(FL_ACCESSIBLE_NODE(accessible));
}

static gchar* fl_accessible_node_get_description(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (self->hint[0] != '\0') {
    return g_strdup(self->hint);
  }
  return g_strdup(self->label[0] == '\0' ? "" : self->tooltip);
}

static gchar* fl_accessible_node_get_help_text(FlAtspiAccessible* accessible) {
  return g_strdup(FL_ACCESSIBLE_NODE(accessible)->tooltip);
}

static gchar* fl_accessible_node_get_accessible_id(
    FlAtspiAccessible* accessible) {
  return g_strdup(FL_ACCESSIBLE_NODE(accessible)->identifier);
}

static void set_state(guint64* states, FlAtspiState state, gboolean value) {
  if (value) {
    *states |= FL_ATSPI_STATE_BIT(state);
  } else {
    *states &= ~FL_ATSPI_STATE_BIT(state);
  }
}

static guint64 fl_accessible_node_get_states(FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  const FlutterSemanticsFlags* flags = &self->flags;
  guint64 states = 0;
  gboolean enabled = flags->is_enabled != kFlutterTristateFalse;
  set_state(&states, FL_ATSPI_STATE_ENABLED, enabled);
  set_state(&states, FL_ATSPI_STATE_SENSITIVE, enabled);
  set_state(&states, FL_ATSPI_STATE_VISIBLE, !flags->is_hidden);
  set_state(&states, FL_ATSPI_STATE_SHOWING, !flags->is_hidden);

  set_state(&states, FL_ATSPI_STATE_FOCUSABLE,
            flags->is_focused != kFlutterTristateNone ||
                has_action(self, kFlutterSemanticsActionFocus));
  set_state(&states, FL_ATSPI_STATE_FOCUSED,
            fl_accessible_node_is_focused(self));

  if (flags->is_checked != kFlutterCheckStateNone) {
    set_state(&states, FL_ATSPI_STATE_CHECKABLE, TRUE);
    set_state(&states, FL_ATSPI_STATE_CHECKED,
              flags->is_checked == kFlutterCheckStateTrue);
    set_state(&states, FL_ATSPI_STATE_INDETERMINATE,
              flags->is_checked == kFlutterCheckStateMixed);
  }
  if (flags->is_toggled != kFlutterTristateNone) {
    set_state(&states, FL_ATSPI_STATE_CHECKABLE, TRUE);
    set_state(&states, FL_ATSPI_STATE_CHECKED,
              flags->is_toggled == kFlutterTristateTrue);
  }
  if (flags->is_selected != kFlutterTristateNone) {
    set_state(&states, FL_ATSPI_STATE_SELECTABLE, TRUE);
    set_state(&states, FL_ATSPI_STATE_SELECTED,
              flags->is_selected == kFlutterTristateTrue);
  }
  if (flags->is_expanded != kFlutterTristateNone) {
    gboolean expanded = flags->is_expanded == kFlutterTristateTrue;
    set_state(&states, FL_ATSPI_STATE_EXPANDABLE, TRUE);
    set_state(&states, FL_ATSPI_STATE_EXPANDED, expanded);
    set_state(&states, FL_ATSPI_STATE_COLLAPSED, !expanded);
  }
  set_state(&states, FL_ATSPI_STATE_REQUIRED,
            flags->is_required == kFlutterTristateTrue);
  if (flags->is_text_field) {
    set_state(&states, FL_ATSPI_STATE_EDITABLE, !flags->is_read_only);
    set_state(&states, FL_ATSPI_STATE_READ_ONLY, flags->is_read_only);
    set_state(&states, FL_ATSPI_STATE_MULTI_LINE, flags->is_multiline);
    set_state(&states, FL_ATSPI_STATE_SINGLE_LINE, !flags->is_multiline);
    set_state(&states, FL_ATSPI_STATE_SELECTABLE_TEXT, TRUE);
  }
  set_state(&states, FL_ATSPI_STATE_HAS_TOOLTIP, self->tooltip[0] != '\0');
  return states;
}

static GHashTable* fl_accessible_node_get_attributes(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  GHashTable* attributes =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  if (self->flags.is_header && self->heading_level > 0) {
    g_hash_table_insert(attributes, g_strdup("level"),
                        g_strdup_printf("%d", self->heading_level));
  }
  if (self->flags.is_live_region) {
    g_hash_table_insert(attributes, g_strdup("live"), g_strdup("polite"));
    g_hash_table_insert(attributes, g_strdup("container-live"),
                        g_strdup("polite"));
  }
  if (self->identifier[0] != '\0') {
    g_hash_table_insert(attributes, g_strdup("id"), g_strdup(self->identifier));
  }
  if (self->flags.is_text_field && self->hint[0] != '\0') {
    g_hash_table_insert(attributes, g_strdup("placeholder-text"),
                        g_strdup(self->hint));
  }
  return attributes;
}

static GPtrArray* fl_accessible_node_get_relations(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  GPtrArray* relations = g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(fl_atspi_relation_free));
  if (get_role(self) == FL_ATSPI_ROLE_RADIO_BUTTON && self->parent != nullptr) {
    FlAtspiRelation* group = fl_atspi_relation_new(FL_ATSPI_RELATION_MEMBER_OF);
    for (guint i = 0; i < self->parent->children->len; i++) {
      FlAccessibleNode* sibling = get_child_node(self->parent, i);
      if (sibling != nullptr &&
          get_role(sibling) == FL_ATSPI_ROLE_RADIO_BUTTON) {
        g_ptr_array_add(group->targets, g_object_ref(sibling));
      }
    }
    g_ptr_array_add(relations, group);
  }
  return relations;
}

static gboolean fl_accessible_node_get_extents(FlAtspiAccessible* accessible,
                                               GdkRectangle* extents) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  gdouble xs[4] = {self->rect.left, self->rect.right, self->rect.right,
                   self->rect.left};
  gdouble ys[4] = {self->rect.top, self->rect.top, self->rect.bottom,
                   self->rect.bottom};
  for (FlAccessibleNode* node = self; node != nullptr; node = node->parent) {
    for (int i = 0; i < 4; i++) {
      transform_point(&node->transform, &xs[i], &ys[i]);
    }
  }
  gdouble ratio = fl_view_accessible_get_pixel_ratio(self->view);
  if (ratio <= 0) {
    ratio = 1;
  }
  gdouble left = xs[0], right = xs[0], top = ys[0], bottom = ys[0];
  for (int i = 1; i < 4; i++) {
    left = MIN(left, xs[i]);
    right = MAX(right, xs[i]);
    top = MIN(top, ys[i]);
    bottom = MAX(bottom, ys[i]);
  }
  extents->x = static_cast<gint>(std::floor(left / ratio));
  extents->y = static_cast<gint>(std::floor(top / ratio));
  extents->width = static_cast<gint>(std::ceil(right / ratio)) - extents->x;
  extents->height = static_cast<gint>(std::ceil(bottom / ratio)) - extents->y;
  return TRUE;
}

static gboolean fl_accessible_node_grab_focus(FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!has_action(self, kFlutterSemanticsActionFocus)) {
    return FALSE;
  }
  send_action(self, kFlutterSemanticsActionFocus, nullptr);
  return TRUE;
}

static gboolean fl_accessible_node_scroll_to(FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!has_action(self, kFlutterSemanticsActionShowOnScreen)) {
    return FALSE;
  }
  send_action(self, kFlutterSemanticsActionShowOnScreen, nullptr);
  return TRUE;
}

static GPtrArray* fl_accessible_node_get_actions(
    FlAtspiAccessible* accessible) {
  g_autoptr(GArray) actions = get_action_list(FL_ACCESSIBLE_NODE(accessible));
  GPtrArray* result = g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(fl_atspi_action_free));
  for (guint i = 0; i < actions->len; i++) {
    NodeAction* action = &g_array_index(actions, NodeAction, i);
    g_ptr_array_add(result,
                    fl_atspi_action_new(action->name, action->description));
  }
  return result;
}

static gboolean fl_accessible_node_do_action(FlAtspiAccessible* accessible,
                                             gsize index) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  g_autoptr(GArray) actions = get_action_list(self);
  if (index >= actions->len) {
    return FALSE;
  }
  NodeAction* action = &g_array_index(actions, NodeAction, index);
  g_autoptr(FlValue) data =
      action->action == kFlutterSemanticsActionCustomAction
          ? fl_value_new_int(action->custom_action_id)
          : nullptr;
  send_action(self, action->action, data);
  return TRUE;
}

static FlAtspiValue* fl_accessible_node_get_value(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!self->flags.is_slider &&
      !has_action(self, kFlutterSemanticsActionIncrease) &&
      !has_action(self, kFlutterSemanticsActionDecrease)) {
    return nullptr;
  }
  gdouble number;
  gboolean percent;
  if (!parse_number(self->value, &number, &percent)) {
    return fl_atspi_value_new(0, 0, 0, 0, self->value);
  }
  return fl_atspi_value_new(MIN(0.0, number),
                            percent || number <= 100 ? 100 : number, 0, number,
                            self->value);
}

static gboolean fl_accessible_node_set_value(FlAtspiAccessible* accessible,
                                             gdouble value) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  g_autoptr(FlAtspiValue) current = fl_accessible_node_get_value(accessible);
  if (current == nullptr) {
    return FALSE;
  }
  self->has_target_value = FALSE;
  if (value == current->current) {
    return TRUE;
  }
  // Flutter can only step the value, so step until the target is reached.
  FlutterSemanticsAction action = value > current->current
                                      ? kFlutterSemanticsActionIncrease
                                      : kFlutterSemanticsActionDecrease;
  if (!has_action(self, action)) {
    return FALSE;
  }
  self->has_target_value = TRUE;
  self->target_value = value;
  self->remaining_steps = kMaxValueSteps;
  send_action(self, action, nullptr);
  return TRUE;
}

static gboolean fl_accessible_node_has_text(FlAtspiAccessible* accessible) {
  return FL_ACCESSIBLE_NODE(accessible)->flags.is_text_field;
}

static gboolean fl_accessible_node_is_editable_text(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  return self->flags.is_text_field && !self->flags.is_read_only;
}

static gchar* fl_accessible_node_get_text(FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  return g_strdup(self->flags.is_text_field ? self->value : "");
}

static gint fl_accessible_node_get_caret_offset(FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!self->flags.is_text_field || self->text_selection_extent < 0) {
    return -1;
  }
  return fl_atspi_text_utf16_to_offset(self->value,
                                       self->text_selection_extent);
}

static gboolean fl_accessible_node_get_selection(FlAtspiAccessible* accessible,
                                                 gint* start,
                                                 gint* end) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!self->flags.is_text_field || self->text_selection_base < 0 ||
      self->text_selection_base == self->text_selection_extent) {
    return FALSE;
  }
  gint base =
      fl_atspi_text_utf16_to_offset(self->value, self->text_selection_base);
  gint extent =
      fl_atspi_text_utf16_to_offset(self->value, self->text_selection_extent);
  *start = MIN(base, extent);
  *end = MAX(base, extent);
  return TRUE;
}

static gboolean fl_accessible_node_set_selection(FlAtspiAccessible* accessible,
                                                 gint start,
                                                 gint end) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!self->flags.is_text_field ||
      !has_action(self, kFlutterSemanticsActionSetSelection)) {
    return FALSE;
  }
  g_autoptr(FlValue) selection = fl_value_new_map();
  fl_value_set_string_take(
      selection, "base",
      fl_value_new_int(fl_atspi_text_offset_to_utf16(self->value, start)));
  fl_value_set_string_take(
      selection, "extent",
      fl_value_new_int(fl_atspi_text_offset_to_utf16(self->value, end)));
  send_action(self, kFlutterSemanticsActionSetSelection, selection);
  return TRUE;
}

static gboolean fl_accessible_node_set_text_contents(
    FlAtspiAccessible* accessible,
    const gchar* text) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  if (!fl_accessible_node_is_editable_text(accessible) ||
      !has_action(self, kFlutterSemanticsActionSetText)) {
    return FALSE;
  }
  g_autoptr(FlValue) data = fl_value_new_string(text);
  send_action(self, kFlutterSemanticsActionSetText, data);
  return TRUE;
}

static gboolean fl_accessible_node_do_clipboard_action(
    FlAtspiAccessible* accessible,
    FlAtspiClipboardAction action) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  FlutterSemanticsAction semantics_action;
  switch (action) {
    case FL_ATSPI_CLIPBOARD_ACTION_COPY:
      semantics_action = kFlutterSemanticsActionCopy;
      break;
    case FL_ATSPI_CLIPBOARD_ACTION_CUT:
      semantics_action = kFlutterSemanticsActionCut;
      break;
    case FL_ATSPI_CLIPBOARD_ACTION_PASTE:
      semantics_action = kFlutterSemanticsActionPaste;
      break;
    default:
      return FALSE;
  }
  if (!has_action(self, semantics_action)) {
    return FALSE;
  }
  send_action(self, semantics_action, nullptr);
  return TRUE;
}

static gboolean fl_accessible_node_is_selection_container(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  for (guint i = 0; i < self->children->len; i++) {
    FlAccessibleNode* child = get_child_node(self, i);
    if (child != nullptr && child->flags.is_selected != kFlutterTristateNone) {
      return TRUE;
    }
  }
  return FALSE;
}

static GArray* fl_accessible_node_get_selected_children(
    FlAtspiAccessible* accessible) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(accessible);
  GArray* selected = g_array_new(FALSE, FALSE, sizeof(guint));
  for (guint i = 0; i < self->children->len; i++) {
    FlAccessibleNode* child = get_child_node(self, i);
    if (child != nullptr && child->flags.is_selected == kFlutterTristateTrue) {
      g_array_append_val(selected, i);
    }
  }
  return selected;
}

static gboolean fl_accessible_node_select_child(FlAtspiAccessible* accessible,
                                                gsize index) {
  FlAccessibleNode* child =
      get_child_node(FL_ACCESSIBLE_NODE(accessible), index);
  if (child == nullptr || child->flags.is_selected == kFlutterTristateNone) {
    return FALSE;
  }
  if (child->flags.is_selected == kFlutterTristateTrue) {
    return TRUE;
  }
  // Selectable items, e.g. tabs and chips, are selected by tapping them.
  if (!has_action(child, kFlutterSemanticsActionTap)) {
    return FALSE;
  }
  send_action(child, kFlutterSemanticsActionTap, nullptr);
  return TRUE;
}

static gboolean fl_accessible_node_is_image(FlAtspiAccessible* accessible) {
  return FL_ACCESSIBLE_NODE(accessible)->flags.is_image;
}

static void fl_accessible_node_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface) {
  iface->get_parent = fl_accessible_node_get_parent;
  iface->get_n_children = fl_accessible_node_get_n_children;
  iface->get_child = fl_accessible_node_get_child;
  iface->get_role = fl_accessible_node_get_role;
  iface->get_name = fl_accessible_node_get_name;
  iface->get_description = fl_accessible_node_get_description;
  iface->get_help_text = fl_accessible_node_get_help_text;
  iface->get_accessible_id = fl_accessible_node_get_accessible_id;
  iface->get_states = fl_accessible_node_get_states;
  iface->get_attributes = fl_accessible_node_get_attributes;
  iface->get_relations = fl_accessible_node_get_relations;
  iface->get_extents = fl_accessible_node_get_extents;
  iface->grab_focus = fl_accessible_node_grab_focus;
  iface->scroll_to = fl_accessible_node_scroll_to;
  iface->get_actions = fl_accessible_node_get_actions;
  iface->do_action = fl_accessible_node_do_action;
  iface->get_value = fl_accessible_node_get_value;
  iface->set_value = fl_accessible_node_set_value;
  iface->has_text = fl_accessible_node_has_text;
  iface->is_editable_text = fl_accessible_node_is_editable_text;
  iface->get_text = fl_accessible_node_get_text;
  iface->get_caret_offset = fl_accessible_node_get_caret_offset;
  iface->get_selection = fl_accessible_node_get_selection;
  iface->set_selection = fl_accessible_node_set_selection;
  iface->set_text_contents = fl_accessible_node_set_text_contents;
  iface->do_clipboard_action = fl_accessible_node_do_clipboard_action;
  iface->is_selection_container = fl_accessible_node_is_selection_container;
  iface->get_selected_children = fl_accessible_node_get_selected_children;
  iface->select_child = fl_accessible_node_select_child;
  iface->is_image = fl_accessible_node_is_image;
}

static void fl_accessible_node_finalize(GObject* object) {
  FlAccessibleNode* self = FL_ACCESSIBLE_NODE(object);

  g_free(self->label);
  g_free(self->hint);
  g_free(self->value);
  g_free(self->tooltip);
  g_free(self->identifier);
  g_array_unref(self->children);
  g_array_unref(self->custom_actions);

  G_OBJECT_CLASS(fl_accessible_node_parent_class)->finalize(object);
}

static void fl_accessible_node_class_init(FlAccessibleNodeClass* klass) {
  G_OBJECT_CLASS(klass)->finalize = fl_accessible_node_finalize;
}

static void fl_accessible_node_init(FlAccessibleNode* self) {
  self->flags.struct_size = sizeof(self->flags);
  self->label = g_strdup("");
  self->hint = g_strdup("");
  self->value = g_strdup("");
  self->tooltip = g_strdup("");
  self->identifier = g_strdup("");
  self->text_selection_base = -1;
  self->text_selection_extent = -1;
  self->children = g_array_new(FALSE, FALSE, sizeof(int32_t));
  self->custom_actions = g_array_new(FALSE, FALSE, sizeof(int32_t));
}

FlAccessibleNode* fl_accessible_node_new(FlViewAccessible* view, int32_t id) {
  FlAccessibleNode* self =
      FL_ACCESSIBLE_NODE(g_object_new(fl_accessible_node_get_type(), nullptr));
  self->view = view;
  self->id = id;
  return self;
}

int32_t fl_accessible_node_get_id(FlAccessibleNode* self) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_NODE(self), -1);
  return self->id;
}

static void set_string(gchar** field, const gchar* value) {
  g_free(*field);
  *field = g_strdup(value != nullptr ? value : "");
}

void fl_accessible_node_update(FlAccessibleNode* self,
                               const FlutterSemanticsNode2* semantics) {
  g_return_if_fail(FL_IS_ACCESSIBLE_NODE(self));

  if (semantics->flags2 != nullptr) {
    self->flags = *semantics->flags2;
  } else {
    self->flags = {};
    self->flags.struct_size = sizeof(self->flags);
  }
  self->actions = semantics->actions;
  set_string(&self->label, semantics->label);
  set_string(&self->hint, semantics->hint);
  set_string(&self->value, semantics->value);
  set_string(&self->tooltip, semantics->tooltip);
  set_string(&self->identifier, semantics->identifier);
  self->text_selection_base = semantics->text_selection_base;
  self->text_selection_extent = semantics->text_selection_extent;
  self->rect = semantics->rect;
  self->transform = semantics->transform;
  g_array_set_size(self->children, 0);
  g_array_append_vals(self->children, semantics->children_in_traversal_order,
                      semantics->child_count);
  g_array_set_size(self->custom_actions, 0);
  g_array_append_vals(self->custom_actions,
                      semantics->custom_accessibility_actions,
                      semantics->custom_accessibility_actions_count);
  self->heading_level = semantics->heading_level;
}

GArray* fl_accessible_node_get_child_ids(FlAccessibleNode* self) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_NODE(self), nullptr);
  return self->children;
}

FlAccessibleNode* fl_accessible_node_get_parent_node(FlAccessibleNode* self) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_NODE(self), nullptr);
  return self->parent;
}

void fl_accessible_node_set_parent_node(FlAccessibleNode* self,
                                        FlAccessibleNode* parent) {
  g_return_if_fail(FL_IS_ACCESSIBLE_NODE(self));
  self->parent = parent;
}

gboolean fl_accessible_node_is_focused(FlAccessibleNode* self) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_NODE(self), FALSE);
  return self->flags.is_focused == kFlutterTristateTrue;
}

gboolean fl_accessible_node_is_live_region(FlAccessibleNode* self) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_NODE(self), FALSE);
  return self->flags.is_live_region;
}

void fl_accessible_node_continue_set_value(FlAccessibleNode* self,
                                           FlAtspiValue* old_value) {
  g_return_if_fail(FL_IS_ACCESSIBLE_NODE(self));

  if (!self->has_target_value) {
    return;
  }
  g_autoptr(FlAtspiValue) current =
      fl_accessible_node_get_value(FL_ATSPI_ACCESSIBLE(self));
  // Stop if the value didn't move, e.g. at a limit.
  if (current == nullptr || old_value == nullptr ||
      current->current == old_value->current || --self->remaining_steps <= 0) {
    self->has_target_value = FALSE;
    return;
  }
  gboolean increasing = current->current > old_value->current;
  if (increasing ? current->current >= self->target_value
                 : current->current <= self->target_value) {
    self->has_target_value = FALSE;
    return;
  }
  FlutterSemanticsAction action = increasing ? kFlutterSemanticsActionIncrease
                                             : kFlutterSemanticsActionDecrease;
  if (!has_action(self, action)) {
    self->has_target_value = FALSE;
    return;
  }
  send_action(self, action, nullptr);
}
