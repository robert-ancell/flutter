// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_ACCESSIBLE_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_ACCESSIBLE_H_

#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * FlAtspiRole:
 *
 * AT-SPI roles (AtspiRole), as sent over D-Bus.
 */
typedef enum {
  FL_ATSPI_ROLE_INVALID = 0,
  FL_ATSPI_ROLE_CHECK_BOX = 7,
  FL_ATSPI_ROLE_DIALOG = 16,
  FL_ATSPI_ROLE_IMAGE = 27,
  FL_ATSPI_ROLE_LABEL = 29,
  FL_ATSPI_ROLE_PANEL = 39,
  FL_ATSPI_ROLE_PASSWORD_TEXT = 40,
  FL_ATSPI_ROLE_BUTTON = 43,
  FL_ATSPI_ROLE_RADIO_BUTTON = 44,
  FL_ATSPI_ROLE_SCROLL_PANE = 49,
  FL_ATSPI_ROLE_SLIDER = 51,
  FL_ATSPI_ROLE_APPLICATION = 75,
  FL_ATSPI_ROLE_ENTRY = 79,
  FL_ATSPI_ROLE_HEADING = 83,
  FL_ATSPI_ROLE_LINK = 88,
  FL_ATSPI_ROLE_SWITCH = 130,
} FlAtspiRole;

/**
 * FlAtspiState:
 *
 * AT-SPI states (AtspiStateType). A set of states is a #guint64 with the bit
 * for each state set, see FL_ATSPI_STATE_BIT().
 */
typedef enum {
  FL_ATSPI_STATE_CHECKED = 4,
  FL_ATSPI_STATE_COLLAPSED = 5,
  FL_ATSPI_STATE_EDITABLE = 7,
  FL_ATSPI_STATE_ENABLED = 8,
  FL_ATSPI_STATE_EXPANDABLE = 9,
  FL_ATSPI_STATE_EXPANDED = 10,
  FL_ATSPI_STATE_FOCUSABLE = 11,
  FL_ATSPI_STATE_FOCUSED = 12,
  FL_ATSPI_STATE_HAS_TOOLTIP = 13,
  FL_ATSPI_STATE_MULTI_LINE = 17,
  FL_ATSPI_STATE_SELECTABLE = 22,
  FL_ATSPI_STATE_SELECTED = 23,
  FL_ATSPI_STATE_SENSITIVE = 24,
  FL_ATSPI_STATE_SHOWING = 25,
  FL_ATSPI_STATE_SINGLE_LINE = 26,
  FL_ATSPI_STATE_VISIBLE = 30,
  FL_ATSPI_STATE_INDETERMINATE = 32,
  FL_ATSPI_STATE_REQUIRED = 33,
  FL_ATSPI_STATE_SELECTABLE_TEXT = 38,
  FL_ATSPI_STATE_CHECKABLE = 41,
  FL_ATSPI_STATE_READ_ONLY = 43,
} FlAtspiState;

#define FL_ATSPI_STATE_BIT(state) (G_GUINT64_CONSTANT(1) << (state))

/**
 * FlAtspiRelationType:
 *
 * AT-SPI relation types (AtspiRelationType).
 */
typedef enum {
  FL_ATSPI_RELATION_MEMBER_OF = 5,
} FlAtspiRelationType;

/**
 * FlAtspiCoordType:
 *
 * AT-SPI coordinate types (AtspiCoordType).
 */
typedef enum {
  FL_ATSPI_COORD_TYPE_SCREEN = 0,
  FL_ATSPI_COORD_TYPE_WINDOW = 1,
  FL_ATSPI_COORD_TYPE_PARENT = 2,
} FlAtspiCoordType;

/**
 * FlAtspiClipboardAction:
 *
 * Clipboard operations on selected text.
 */
typedef enum {
  FL_ATSPI_CLIPBOARD_ACTION_COPY,
  FL_ATSPI_CLIPBOARD_ACTION_CUT,
  FL_ATSPI_CLIPBOARD_ACTION_PASTE,
} FlAtspiClipboardAction;

/**
 * FlAtspiAction:
 * @name: the name of the action, e.g. "click".
 * @description: a description of the action.
 *
 * An action that can be performed on an object.
 */
typedef struct {
  gchar* name;
  gchar* description;
} FlAtspiAction;

FlAtspiAction* fl_atspi_action_new(const gchar* name, const gchar* description);

void fl_atspi_action_free(FlAtspiAction* action);

/**
 * FlAtspiRelation:
 * @type: the type of relation.
 * @targets: (element-type FlAtspiAccessible): the related objects.
 *
 * A relation from an object to other objects.
 */
typedef struct {
  FlAtspiRelationType type;
  GPtrArray* targets;
} FlAtspiRelation;

FlAtspiRelation* fl_atspi_relation_new(FlAtspiRelationType type);

void fl_atspi_relation_free(FlAtspiRelation* relation);

/**
 * FlAtspiValue:
 *
 * The value of a range, e.g. a slider.
 */
typedef struct {
  gdouble minimum;
  gdouble maximum;
  gdouble increment;
  gdouble current;
  gchar* text;
} FlAtspiValue;

FlAtspiValue* fl_atspi_value_new(gdouble minimum,
                                 gdouble maximum,
                                 gdouble increment,
                                 gdouble current,
                                 const gchar* text);

void fl_atspi_value_free(FlAtspiValue* value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlAtspiValue, fl_atspi_value_free)

G_DECLARE_INTERFACE(FlAtspiAccessible,
                    fl_atspi_accessible,
                    FL,
                    ATSPI_ACCESSIBLE,
                    GObject);

/**
 * FlAtspiAccessible:
 *
 * #FlAtspiAccessible is an interface for objects exposed to assistive
 * technologies over AT-SPI. Text offsets are in Unicode characters.
 *
 * Only get_parent, get_n_children, get_child, get_role and
 * get_states are required, other methods default to an object without that
 * feature.
 */
struct _FlAtspiAccessibleInterface {
  GTypeInterface g_iface;

  FlAtspiAccessible* (*get_parent)(FlAtspiAccessible* self);
  gsize (*get_n_children)(FlAtspiAccessible* self);
  FlAtspiAccessible* (*get_child)(FlAtspiAccessible* self, gsize index);
  FlAtspiRole (*get_role)(FlAtspiAccessible* self);
  gchar* (*get_name)(FlAtspiAccessible* self);
  gchar* (*get_description)(FlAtspiAccessible* self);
  gchar* (*get_help_text)(FlAtspiAccessible* self);
  gchar* (*get_accessible_id)(FlAtspiAccessible* self);
  guint64 (*get_states)(FlAtspiAccessible* self);
  GHashTable* (*get_attributes)(FlAtspiAccessible* self);
  GPtrArray* (*get_relations)(FlAtspiAccessible* self);

  gboolean (*get_extents)(FlAtspiAccessible* self, GdkRectangle* extents);
  void (*get_origin)(FlAtspiAccessible* self,
                     FlAtspiCoordType coord_type,
                     gint* x,
                     gint* y);
  gboolean (*grab_focus)(FlAtspiAccessible* self);
  gboolean (*scroll_to)(FlAtspiAccessible* self);

  GPtrArray* (*get_actions)(FlAtspiAccessible* self);
  gboolean (*do_action)(FlAtspiAccessible* self, gsize index);

  FlAtspiValue* (*get_value)(FlAtspiAccessible* self);
  gboolean (*set_value)(FlAtspiAccessible* self, gdouble value);

  gboolean (*has_text)(FlAtspiAccessible* self);
  gboolean (*is_editable_text)(FlAtspiAccessible* self);
  gchar* (*get_text)(FlAtspiAccessible* self);
  gint (*get_caret_offset)(FlAtspiAccessible* self);
  gboolean (*get_selection)(FlAtspiAccessible* self, gint* start, gint* end);
  gboolean (*set_selection)(FlAtspiAccessible* self, gint start, gint end);
  gboolean (*set_text_contents)(FlAtspiAccessible* self, const gchar* text);
  gboolean (*do_clipboard_action)(FlAtspiAccessible* self,
                                  FlAtspiClipboardAction action);

  gboolean (*is_selection_container)(FlAtspiAccessible* self);
  GArray* (*get_selected_children)(FlAtspiAccessible* self);
  gboolean (*select_child)(FlAtspiAccessible* self, gsize index);

  gboolean (*is_image)(FlAtspiAccessible* self);
};

/**
 * fl_atspi_accessible_get_parent:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer none): the parent or %NULL for the application root.
 */
FlAtspiAccessible* fl_atspi_accessible_get_parent(
    FlAtspiAccessible* accessible);

gsize fl_atspi_accessible_get_n_children(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_child:
 * @accessible: an #FlAtspiAccessible.
 * @index: the index of the child.
 *
 * Returns: (transfer none): the child or %NULL.
 */
FlAtspiAccessible* fl_atspi_accessible_get_child(FlAtspiAccessible* accessible,
                                                 gsize index);

/**
 * fl_atspi_accessible_get_index_in_parent:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: the index of @accessible in its parent's children, or -1.
 */
gint fl_atspi_accessible_get_index_in_parent(FlAtspiAccessible* accessible);

FlAtspiRole fl_atspi_accessible_get_role(FlAtspiAccessible* accessible);

gchar* fl_atspi_accessible_get_name(FlAtspiAccessible* accessible);

gchar* fl_atspi_accessible_get_description(FlAtspiAccessible* accessible);

gchar* fl_atspi_accessible_get_help_text(FlAtspiAccessible* accessible);

gchar* fl_atspi_accessible_get_accessible_id(FlAtspiAccessible* accessible);

guint64 fl_atspi_accessible_get_states(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_attributes:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer full) (element-type utf8 utf8): the object attributes.
 */
GHashTable* fl_atspi_accessible_get_attributes(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_relations:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer full) (element-type FlAtspiRelation): the relations.
 */
GPtrArray* fl_atspi_accessible_get_relations(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_extents:
 * @accessible: an #FlAtspiAccessible.
 * @extents: (out): location to write the extents in logical pixels relative
 * to the window content.
 *
 * Returns: %TRUE if @accessible is a visible component.
 */
gboolean fl_atspi_accessible_get_extents(FlAtspiAccessible* accessible,
                                         GdkRectangle* extents);

/**
 * fl_atspi_accessible_get_origin:
 * @accessible: an #FlAtspiAccessible for a window.
 * @coord_type: the coordinate system to use.
 * @x: (out): location to write the x position.
 * @y: (out): location to write the y position.
 *
 * Gets the position of the window content in screen or toplevel window
 * coordinates.
 */
void fl_atspi_accessible_get_origin(FlAtspiAccessible* accessible,
                                    FlAtspiCoordType coord_type,
                                    gint* x,
                                    gint* y);

gboolean fl_atspi_accessible_grab_focus(FlAtspiAccessible* accessible);

gboolean fl_atspi_accessible_scroll_to(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_accessible_at_point:
 * @accessible: an #FlAtspiAccessible.
 * @x: x position relative to the window content.
 * @y: y position relative to the window content.
 *
 * Returns: (transfer none): the deepest visible descendant containing the
 * point, @accessible if no descendant contains it, or %NULL if @accessible
 * doesn't contain it.
 */
FlAtspiAccessible* fl_atspi_accessible_get_accessible_at_point(
    FlAtspiAccessible* accessible,
    gint x,
    gint y);

/**
 * fl_atspi_accessible_get_actions:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer full) (element-type FlAtspiAction): the actions.
 */
GPtrArray* fl_atspi_accessible_get_actions(FlAtspiAccessible* accessible);

gboolean fl_atspi_accessible_do_action(FlAtspiAccessible* accessible,
                                       gsize index);

/**
 * fl_atspi_accessible_get_value:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer full): the value of @accessible or %NULL if it is not a
 * range.
 */
FlAtspiValue* fl_atspi_accessible_get_value(FlAtspiAccessible* accessible);

gboolean fl_atspi_accessible_set_value(FlAtspiAccessible* accessible,
                                       gdouble value);

gboolean fl_atspi_accessible_has_text(FlAtspiAccessible* accessible);

gboolean fl_atspi_accessible_is_editable_text(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_text:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: the text as UTF-8.
 */
gchar* fl_atspi_accessible_get_text(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_caret_offset:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: the caret offset, or -1 if there is no caret.
 */
gint fl_atspi_accessible_get_caret_offset(FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_selection:
 * @accessible: an #FlAtspiAccessible.
 * @start: (out): location to write the start of the selection.
 * @end: (out): location to write the end of the selection.
 *
 * Returns: %TRUE if there is a non-empty text selection.
 */
gboolean fl_atspi_accessible_get_selection(FlAtspiAccessible* accessible,
                                           gint* start,
                                           gint* end);

gboolean fl_atspi_accessible_set_selection(FlAtspiAccessible* accessible,
                                           gint start,
                                           gint end);

gboolean fl_atspi_accessible_set_text_contents(FlAtspiAccessible* accessible,
                                               const gchar* text);

gboolean fl_atspi_accessible_do_clipboard_action(FlAtspiAccessible* accessible,
                                                 FlAtspiClipboardAction action);

gboolean fl_atspi_accessible_is_selection_container(
    FlAtspiAccessible* accessible);

/**
 * fl_atspi_accessible_get_selected_children:
 * @accessible: an #FlAtspiAccessible.
 *
 * Returns: (transfer full) (element-type guint): the indexes of the selected
 * children, in order.
 */
GArray* fl_atspi_accessible_get_selected_children(
    FlAtspiAccessible* accessible);

gboolean fl_atspi_accessible_select_child(FlAtspiAccessible* accessible,
                                          gsize index);

gboolean fl_atspi_accessible_is_image(FlAtspiAccessible* accessible);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_ACCESSIBLE_H_
