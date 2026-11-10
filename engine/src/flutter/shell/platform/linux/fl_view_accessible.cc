// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_view_accessible.h"

#include "flutter/shell/platform/linux/fl_engine_private.h"
#include "flutter/shell/platform/linux/fl_socket_accessible.h"

// The states reported in StateChanged events.
static const FlAtspiState kReportedStates[] = {
    FL_ATSPI_STATE_CHECKED,       FL_ATSPI_STATE_COLLAPSED,
    FL_ATSPI_STATE_EDITABLE,      FL_ATSPI_STATE_ENABLED,
    FL_ATSPI_STATE_EXPANDABLE,    FL_ATSPI_STATE_EXPANDED,
    FL_ATSPI_STATE_FOCUSABLE,     FL_ATSPI_STATE_MULTI_LINE,
    FL_ATSPI_STATE_SELECTABLE,    FL_ATSPI_STATE_SELECTED,
    FL_ATSPI_STATE_SENSITIVE,     FL_ATSPI_STATE_SHOWING,
    FL_ATSPI_STATE_SINGLE_LINE,   FL_ATSPI_STATE_VISIBLE,
    FL_ATSPI_STATE_INDETERMINATE, FL_ATSPI_STATE_REQUIRED,
    FL_ATSPI_STATE_CHECKABLE,     FL_ATSPI_STATE_READ_ONLY,
};

struct _FlViewAccessible {
  GObject parent_instance;

  // The engine the view is running on.
  GWeakRef engine;

  // The Flutter view ID.
  FlutterViewId view_id;

  // The widget showing the view.
  GWeakRef widget;

  FlAtspiApplication* application;

  // The semantics nodes, by ID.
  GHashTable* nodes;

  // The custom actions, by ID.
  GHashTable* custom_actions;

  // The ID of the node with focus, or -1.
  int32_t focused_id;
};

// A custom action.
typedef struct {
  gchar* label;
  // The standard action this action describes, or 0.
  FlutterSemanticsAction override_action;
} CustomAction;

// The properties of a node that changes are reported for.
typedef struct {
  FlAtspiRole role;
  gchar* name;
  gchar* description;
  guint64 states;
  FlAtspiValue* value;
  gchar* text;
  gint caret;
  gint selection_start;
  gint selection_end;
  GArray* children;
} Snapshot;

// A node changed in an update.
typedef struct {
  FlAccessibleNode* node;
  // The properties before the update, or nullptr for a new node.
  Snapshot* snapshot;
} UpdatedNode;

static void fl_view_accessible_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlViewAccessible,
    fl_view_accessible,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(fl_atspi_accessible_get_type(),
                          fl_view_accessible_atspi_accessible_iface_init))

static void custom_action_free(CustomAction* action) {
  g_free(action->label);
  g_free(action);
}

static Snapshot* snapshot_new(FlAccessibleNode* node) {
  FlAtspiAccessible* accessible = FL_ATSPI_ACCESSIBLE(node);
  Snapshot* snapshot = g_new0(Snapshot, 1);
  snapshot->role = fl_atspi_accessible_get_role(accessible);
  snapshot->name = fl_atspi_accessible_get_name(accessible);
  snapshot->description = fl_atspi_accessible_get_description(accessible);
  snapshot->states = fl_atspi_accessible_get_states(accessible);
  snapshot->value = fl_atspi_accessible_get_value(accessible);
  snapshot->text = fl_atspi_accessible_get_text(accessible);
  snapshot->caret = fl_atspi_accessible_get_caret_offset(accessible);
  snapshot->selection_start = snapshot->selection_end = snapshot->caret;
  fl_atspi_accessible_get_selection(accessible, &snapshot->selection_start,
                                    &snapshot->selection_end);
  snapshot->children = g_array_copy(fl_accessible_node_get_child_ids(node));
  return snapshot;
}

static void snapshot_free(Snapshot* snapshot) {
  g_free(snapshot->name);
  g_free(snapshot->description);
  g_clear_pointer(&snapshot->value, fl_atspi_value_free);
  g_free(snapshot->text);
  g_array_unref(snapshot->children);
  g_free(snapshot);
}

static void updated_node_free(UpdatedNode* updated) {
  g_object_unref(updated->node);
  g_clear_pointer(&updated->snapshot, snapshot_free);
  g_free(updated);
}

static gboolean array_contains(GArray* array, int32_t value) {
  for (guint i = 0; i < array->len; i++) {
    if (g_array_index(array, int32_t, i) == value) {
      return TRUE;
    }
  }
  return FALSE;
}

// Emits events for the text changing from |old_text| to |new_text|.
static void emit_text_changes(FlViewAccessible* self,
                              FlAccessibleNode* node,
                              const gchar* old_text,
                              const gchar* new_text) {
  glong old_length, new_length;
  g_autofree gunichar* old_chars =
      g_utf8_to_ucs4_fast(old_text, -1, &old_length);
  g_autofree gunichar* new_chars =
      g_utf8_to_ucs4_fast(new_text, -1, &new_length);
  glong prefix = 0;
  while (prefix < old_length && prefix < new_length &&
         old_chars[prefix] == new_chars[prefix]) {
    prefix++;
  }
  glong suffix = 0;
  while (suffix < old_length - prefix && suffix < new_length - prefix &&
         old_chars[old_length - 1 - suffix] ==
             new_chars[new_length - 1 - suffix]) {
    suffix++;
  }
  glong removed_length = old_length - prefix - suffix;
  if (removed_length > 0) {
    g_autofree gchar* removed = g_ucs4_to_utf8(
        old_chars + prefix, removed_length, nullptr, nullptr, nullptr);
    fl_atspi_application_emit_text_changed(
        self->application, FL_ATSPI_ACCESSIBLE(node), FALSE, prefix, removed);
  }
  glong inserted_length = new_length - prefix - suffix;
  if (inserted_length > 0) {
    g_autofree gchar* inserted = g_ucs4_to_utf8(
        new_chars + prefix, inserted_length, nullptr, nullptr, nullptr);
    fl_atspi_application_emit_text_changed(
        self->application, FL_ATSPI_ACCESSIBLE(node), TRUE, prefix, inserted);
  }
}

// Emits events for the changes to |node| since |old|.
static void emit_changes(FlViewAccessible* self,
                         FlAccessibleNode* node,
                         Snapshot* old) {
  FlAtspiAccessible* accessible = FL_ATSPI_ACCESSIBLE(node);

  GArray* children = fl_accessible_node_get_child_ids(node);
  for (guint i = 0; i < children->len; i++) {
    int32_t child_id = g_array_index(children, int32_t, i);
    FlAccessibleNode* child = fl_view_accessible_lookup_node(self, child_id);
    if (!array_contains(old->children, child_id) && child != nullptr) {
      fl_atspi_application_emit_children_changed(
          self->application, accessible, TRUE, i, FL_ATSPI_ACCESSIBLE(child));
    }
  }

  g_autofree gchar* name = fl_atspi_accessible_get_name(accessible);
  if (g_strcmp0(name, old->name) != 0) {
    fl_atspi_application_emit_property_changed(self->application, accessible,
                                               FL_ATSPI_PROPERTY_NAME);
    if (fl_accessible_node_is_live_region(node) && name[0] != '\0') {
      fl_atspi_application_emit_announcement(self->application, accessible,
                                             name, FALSE);
    }
  }
  g_autofree gchar* description =
      fl_atspi_accessible_get_description(accessible);
  if (g_strcmp0(description, old->description) != 0) {
    fl_atspi_application_emit_property_changed(self->application, accessible,
                                               FL_ATSPI_PROPERTY_DESCRIPTION);
  }
  FlAtspiRole role = fl_atspi_accessible_get_role(accessible);
  if (role != old->role) {
    fl_atspi_application_emit_property_changed(self->application, accessible,
                                               FL_ATSPI_PROPERTY_ROLE);
  }

  guint64 states = fl_atspi_accessible_get_states(accessible);
  for (FlAtspiState state : kReportedStates) {
    gboolean value = (states & FL_ATSPI_STATE_BIT(state)) != 0;
    if (value != ((old->states & FL_ATSPI_STATE_BIT(state)) != 0)) {
      fl_atspi_application_emit_state_changed(self->application, accessible,
                                              state, value);
    }
  }

  g_autoptr(FlAtspiValue) value = fl_atspi_accessible_get_value(accessible);
  if (value != nullptr &&
      (old->value == nullptr || value->current != old->value->current ||
       value->minimum != old->value->minimum ||
       value->maximum != old->value->maximum ||
       value->increment != old->value->increment ||
       g_strcmp0(value->text, old->value->text) != 0)) {
    fl_atspi_application_emit_property_changed(self->application, accessible,
                                               FL_ATSPI_PROPERTY_VALUE);
  }

  if (fl_atspi_accessible_has_text(accessible)) {
    g_autofree gchar* text = fl_atspi_accessible_get_text(accessible);
    if (g_strcmp0(text, old->text) != 0) {
      emit_text_changes(self, node, old->text, text);
    }
    gint caret = fl_atspi_accessible_get_caret_offset(accessible);
    gint selection_start = caret, selection_end = caret;
    fl_atspi_accessible_get_selection(accessible, &selection_start,
                                      &selection_end);
    // Moving the caret without a selection isn't a selection change.
    gboolean had_selection = old->selection_start != old->selection_end;
    gboolean has_selection = selection_start != selection_end;
    if ((had_selection || has_selection) &&
        (selection_start != old->selection_start ||
         selection_end != old->selection_end)) {
      fl_atspi_application_emit_text_selection_changed(self->application,
                                                       accessible);
    }
    if (caret != old->caret && caret >= 0) {
      fl_atspi_application_emit_text_caret_moved(self->application, accessible,
                                                 caret);
    }
  }
}

// Removes the node with |id| and its descendants that are not in |kept|.
static void remove_subtree(FlViewAccessible* self,
                           int32_t id,
                           GHashTable* kept) {
  g_autoptr(FlAccessibleNode) node = nullptr;
  if (!g_hash_table_steal_extended(self->nodes, GINT_TO_POINTER(id), nullptr,
                                   reinterpret_cast<gpointer*>(&node))) {
    return;
  }
  if (self->focused_id == id) {
    self->focused_id = -1;
  }
  GArray* children = fl_accessible_node_get_child_ids(node);
  for (guint i = 0; i < children->len; i++) {
    int32_t child_id = g_array_index(children, int32_t, i);
    FlAccessibleNode* child = fl_view_accessible_lookup_node(self, child_id);
    if (child == nullptr) {
      continue;
    }
    // Children that moved to another parent are kept.
    if (g_hash_table_contains(kept, GINT_TO_POINTER(child_id))) {
      if (fl_accessible_node_get_parent_node(child) == node) {
        fl_accessible_node_set_parent_node(child, nullptr);
      }
      continue;
    }
    remove_subtree(self, child_id, kept);
  }
}

static FlAtspiAccessible* fl_view_accessible_get_parent(
    FlAtspiAccessible* accessible) {
  return FL_ATSPI_ACCESSIBLE(FL_VIEW_ACCESSIBLE(accessible)->application);
}

static gsize fl_view_accessible_get_n_children(FlAtspiAccessible* accessible) {
  return fl_view_accessible_lookup_node(FL_VIEW_ACCESSIBLE(accessible), 0) !=
                 nullptr
             ? 1
             : 0;
}

static FlAtspiAccessible* fl_view_accessible_get_child(
    FlAtspiAccessible* accessible,
    gsize index) {
  FlAccessibleNode* root = index == 0 ? fl_view_accessible_lookup_node(
                                            FL_VIEW_ACCESSIBLE(accessible), 0)
                                      : nullptr;
  return root != nullptr ? FL_ATSPI_ACCESSIBLE(root) : nullptr;
}

static FlAtspiRole fl_view_accessible_get_role(FlAtspiAccessible* accessible) {
  return FL_ATSPI_ROLE_PANEL;
}

static guint64 fl_view_accessible_get_states(FlAtspiAccessible* accessible) {
  return FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_ENABLED) |
         FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_SENSITIVE) |
         FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_SHOWING) |
         FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_VISIBLE) |
         FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_FOCUSABLE);
}

static gboolean fl_view_accessible_get_extents(FlAtspiAccessible* accessible,
                                               GdkRectangle* extents) {
  FlViewAccessible* self = FL_VIEW_ACCESSIBLE(accessible);
  g_autoptr(GtkWidget) widget = GTK_WIDGET(g_weak_ref_get(&self->widget));
  if (widget != nullptr) {
    extents->width = gtk_widget_get_allocated_width(widget);
    extents->height = gtk_widget_get_allocated_height(widget);
  }
  return TRUE;
}

// Gets the position of the widget on the screen or in its toplevel window.
static void fl_view_accessible_get_origin(FlAtspiAccessible* accessible,
                                          FlAtspiCoordType coord_type,
                                          gint* x,
                                          gint* y) {
  FlViewAccessible* self = FL_VIEW_ACCESSIBLE(accessible);
  g_autoptr(GtkWidget) widget = GTK_WIDGET(g_weak_ref_get(&self->widget));
  if (widget == nullptr) {
    return;
  }
  GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
  gtk_widget_translate_coordinates(widget, toplevel, 0, 0, x, y);
  GdkWindow* window = gtk_widget_get_window(toplevel);
  if (coord_type == FL_ATSPI_COORD_TYPE_SCREEN && window != nullptr) {
    gint origin_x = 0, origin_y = 0;
    gdk_window_get_origin(window, &origin_x, &origin_y);
    *x += origin_x;
    *y += origin_y;
  }
}

static void fl_view_accessible_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface) {
  iface->get_parent = fl_view_accessible_get_parent;
  iface->get_n_children = fl_view_accessible_get_n_children;
  iface->get_child = fl_view_accessible_get_child;
  iface->get_role = fl_view_accessible_get_role;
  iface->get_states = fl_view_accessible_get_states;
  iface->get_extents = fl_view_accessible_get_extents;
  iface->get_origin = fl_view_accessible_get_origin;
}

static void fl_view_accessible_dispose(GObject* object) {
  FlViewAccessible* self = FL_VIEW_ACCESSIBLE(object);

  if (self->application != nullptr) {
    fl_atspi_application_remove_window(self->application,
                                       FL_ATSPI_ACCESSIBLE(self));
    g_clear_object(&self->application);
  }
  g_hash_table_remove_all(self->nodes);
  g_hash_table_remove_all(self->custom_actions);

  G_OBJECT_CLASS(fl_view_accessible_parent_class)->dispose(object);
}

static void fl_view_accessible_finalize(GObject* object) {
  FlViewAccessible* self = FL_VIEW_ACCESSIBLE(object);

  g_weak_ref_clear(&self->engine);
  g_weak_ref_clear(&self->widget);
  g_hash_table_unref(self->nodes);
  g_hash_table_unref(self->custom_actions);

  G_OBJECT_CLASS(fl_view_accessible_parent_class)->finalize(object);
}

static void fl_view_accessible_class_init(FlViewAccessibleClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_view_accessible_dispose;
  G_OBJECT_CLASS(klass)->finalize = fl_view_accessible_finalize;
}

static void fl_view_accessible_init(FlViewAccessible* self) {
  self->nodes =
      g_hash_table_new_full(g_direct_hash, g_direct_equal, nullptr,
                            reinterpret_cast<GDestroyNotify>(g_object_unref));
  self->custom_actions = g_hash_table_new_full(
      g_direct_hash, g_direct_equal, nullptr,
      reinterpret_cast<GDestroyNotify>(custom_action_free));
  self->focused_id = -1;
}

FlViewAccessible* fl_view_accessible_new(FlEngine* engine,
                                         FlAtspiApplication* application,
                                         FlutterViewId view_id) {
  FlViewAccessible* self =
      FL_VIEW_ACCESSIBLE(g_object_new(fl_view_accessible_get_type(), nullptr));

  g_weak_ref_init(&self->engine, engine);
  self->view_id = view_id;
  g_weak_ref_init(&self->widget, nullptr);
  self->application = FL_ATSPI_APPLICATION(g_object_ref(application));
  fl_atspi_application_add_window(self->application, FL_ATSPI_ACCESSIBLE(self));

  return self;
}

void fl_view_accessible_embed(FlViewAccessible* self, GtkWidget* widget) {
  g_return_if_fail(FL_IS_VIEW_ACCESSIBLE(self));

  g_weak_ref_set(&self->widget, widget);

  AtkObject* accessible = gtk_widget_get_accessible(widget);
  if (!FL_IS_SOCKET_ACCESSIBLE(accessible)) {
    return;
  }
  g_autoptr(AtkObject) socket = atk_object_ref_accessible_child(accessible, 0);
  fl_atspi_application_set_window_socket(
      self->application, FL_ATSPI_ACCESSIBLE(self), ATK_SOCKET(socket));
}

void fl_view_accessible_handle_update_semantics(
    FlViewAccessible* self,
    const FlutterSemanticsUpdate2* update) {
  g_return_if_fail(FL_IS_VIEW_ACCESSIBLE(self));

  for (size_t i = 0; i < update->custom_action_count; i++) {
    const FlutterSemanticsCustomAction2* action = update->custom_actions[i];
    CustomAction* custom_action = g_new0(CustomAction, 1);
    custom_action->label =
        g_strdup(action->label != nullptr ? action->label : "");
    custom_action->override_action = action->override_action;
    g_hash_table_insert(self->custom_actions, GINT_TO_POINTER(action->id),
                        custom_action);
  }

  // Apply the new properties, remembering the old ones.
  g_autoptr(GPtrArray) updated = g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(updated_node_free));
  gboolean root_added = FALSE;
  for (size_t i = 0; i < update->node_count; i++) {
    const FlutterSemanticsNode2* data = update->nodes[i];
    UpdatedNode* entry = g_new0(UpdatedNode, 1);
    FlAccessibleNode* node = fl_view_accessible_lookup_node(self, data->id);
    if (node != nullptr) {
      entry->snapshot = snapshot_new(node);
    } else {
      node = fl_accessible_node_new(self, data->id);
      g_hash_table_insert(self->nodes, GINT_TO_POINTER(data->id), node);
      root_added |= data->id == 0;
    }
    entry->node = FL_ACCESSIBLE_NODE(g_object_ref(node));
    fl_accessible_node_update(node, data);
    g_ptr_array_add(updated, entry);
  }

  // Link children to their parents. Nodes that are still children are kept,
  // even if they moved.
  g_autoptr(GHashTable) kept = g_hash_table_new(g_direct_hash, g_direct_equal);
  for (guint i = 0; i < updated->len; i++) {
    UpdatedNode* entry = static_cast<UpdatedNode*>(updated->pdata[i]);
    GArray* children = fl_accessible_node_get_child_ids(entry->node);
    for (guint j = 0; j < children->len; j++) {
      int32_t child_id = g_array_index(children, int32_t, j);
      g_hash_table_add(kept, GINT_TO_POINTER(child_id));
      FlAccessibleNode* child = fl_view_accessible_lookup_node(self, child_id);
      if (child != nullptr) {
        fl_accessible_node_set_parent_node(child, entry->node);
      }
    }
  }

  // Report and remove children that are no longer present. The objects are
  // removed after reporting, so the events can refer to them.
  g_autoptr(GArray) removed = g_array_new(FALSE, FALSE, sizeof(int32_t));
  for (guint i = 0; i < updated->len; i++) {
    UpdatedNode* entry = static_cast<UpdatedNode*>(updated->pdata[i]);
    if (entry->snapshot == nullptr) {
      continue;
    }
    GArray* new_children = fl_accessible_node_get_child_ids(entry->node);
    GArray* old_children = entry->snapshot->children;
    for (guint j = old_children->len; j > 0; j--) {
      int32_t child_id = g_array_index(old_children, int32_t, j - 1);
      if (array_contains(new_children, child_id)) {
        continue;
      }
      FlAccessibleNode* child = fl_view_accessible_lookup_node(self, child_id);
      if (child != nullptr) {
        fl_atspi_application_emit_children_changed(
            self->application, FL_ATSPI_ACCESSIBLE(entry->node), FALSE, j - 1,
            FL_ATSPI_ACCESSIBLE(child));
      }
      if (!g_hash_table_contains(kept, GINT_TO_POINTER(child_id))) {
        g_array_append_val(removed, child_id);
      }
    }
  }
  for (guint i = 0; i < removed->len; i++) {
    remove_subtree(self, g_array_index(removed, int32_t, i), kept);
  }

  if (root_added) {
    fl_atspi_application_emit_children_changed(
        self->application, FL_ATSPI_ACCESSIBLE(self), TRUE, 0,
        FL_ATSPI_ACCESSIBLE(fl_view_accessible_lookup_node(self, 0)));
  }

  g_autoptr(GPtrArray) selection_changed = g_ptr_array_new();
  for (guint i = 0; i < updated->len; i++) {
    UpdatedNode* entry = static_cast<UpdatedNode*>(updated->pdata[i]);
    if (entry->snapshot == nullptr) {
      continue;
    }
    emit_changes(self, entry->node, entry->snapshot);
    guint64 selected_bit = FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_SELECTED);
    gboolean selected =
        (fl_atspi_accessible_get_states(FL_ATSPI_ACCESSIBLE(entry->node)) &
         selected_bit) != 0;
    gboolean was_selected = (entry->snapshot->states & selected_bit) != 0;
    FlAccessibleNode* parent = fl_accessible_node_get_parent_node(entry->node);
    if (selected != was_selected && parent != nullptr &&
        !g_ptr_array_find(selection_changed, parent, nullptr)) {
      g_ptr_array_add(selection_changed, parent);
    }
    fl_accessible_node_continue_set_value(entry->node, entry->snapshot->value);
  }
  for (guint i = 0; i < selection_changed->len; i++) {
    fl_atspi_application_emit_selection_changed(
        self->application,
        FL_ATSPI_ACCESSIBLE(g_ptr_array_index(selection_changed, i)));
  }

  // Report focus last, after the focused object has been described.
  int32_t focused_id = self->focused_id;
  FlAccessibleNode* old_focus =
      fl_view_accessible_lookup_node(self, self->focused_id);
  if (old_focus == nullptr || !fl_accessible_node_is_focused(old_focus)) {
    focused_id = -1;
  }
  for (guint i = 0; i < updated->len; i++) {
    UpdatedNode* entry = static_cast<UpdatedNode*>(updated->pdata[i]);
    if (fl_accessible_node_is_focused(entry->node) &&
        (focused_id == -1 || entry->snapshot == nullptr ||
         (entry->snapshot->states &
          FL_ATSPI_STATE_BIT(FL_ATSPI_STATE_FOCUSED)) == 0)) {
      focused_id = fl_accessible_node_get_id(entry->node);
    }
  }
  if (focused_id != self->focused_id) {
    if (old_focus != nullptr) {
      fl_atspi_application_emit_state_changed(self->application,
                                              FL_ATSPI_ACCESSIBLE(old_focus),
                                              FL_ATSPI_STATE_FOCUSED, FALSE);
    }
    self->focused_id = focused_id;
    FlAccessibleNode* new_focus =
        fl_view_accessible_lookup_node(self, self->focused_id);
    if (new_focus != nullptr) {
      fl_atspi_application_emit_state_changed(self->application,
                                              FL_ATSPI_ACCESSIBLE(new_focus),
                                              FL_ATSPI_STATE_FOCUSED, TRUE);
    }
  }
}

void fl_view_accessible_send_announcement(FlViewAccessible* self,
                                          const char* message,
                                          gboolean assertive) {
  g_return_if_fail(FL_IS_VIEW_ACCESSIBLE(self));

  fl_atspi_application_emit_announcement(
      self->application, FL_ATSPI_ACCESSIBLE(self), message, assertive);
}

FlAccessibleNode* fl_view_accessible_lookup_node(FlViewAccessible* self,
                                                 int32_t id) {
  g_return_val_if_fail(FL_IS_VIEW_ACCESSIBLE(self), nullptr);
  return FL_ACCESSIBLE_NODE(
      g_hash_table_lookup(self->nodes, GINT_TO_POINTER(id)));
}

gboolean fl_view_accessible_get_custom_action(
    FlViewAccessible* self,
    int32_t id,
    const gchar** label,
    FlutterSemanticsAction* override_action) {
  g_return_val_if_fail(FL_IS_VIEW_ACCESSIBLE(self), FALSE);
  CustomAction* action = static_cast<CustomAction*>(
      g_hash_table_lookup(self->custom_actions, GINT_TO_POINTER(id)));
  if (action == nullptr) {
    return FALSE;
  }
  *label = action->label;
  *override_action = action->override_action;
  return TRUE;
}

gdouble fl_view_accessible_get_pixel_ratio(FlViewAccessible* self) {
  g_return_val_if_fail(FL_IS_VIEW_ACCESSIBLE(self), 1.0);
  g_autoptr(GtkWidget) widget = GTK_WIDGET(g_weak_ref_get(&self->widget));
  return widget != nullptr ? gtk_widget_get_scale_factor(widget) : 1.0;
}

void fl_view_accessible_dispatch_action(FlViewAccessible* self,
                                        int32_t node_id,
                                        FlutterSemanticsAction action,
                                        GBytes* data) {
  g_return_if_fail(FL_IS_VIEW_ACCESSIBLE(self));
  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return;
  }
  fl_engine_dispatch_semantics_action(engine, self->view_id, node_id, action,
                                      data);
}
