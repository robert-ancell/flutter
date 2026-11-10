// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_object.h"

#include <clocale>

#include "flutter/shell/platform/linux/fl_atspi_application_private.h"
#include "flutter/shell/platform/linux/fl_atspi_dbus.h"
#include "flutter/shell/platform/linux/fl_atspi_text.h"

static constexpr char kNullPath[] = "/org/a11y/atspi/null";

// AT-SPI component layers (AtspiComponentLayer).
static constexpr guint32 kLayerWidget = 3;
static constexpr guint32 kLayerWindow = 7;

// The AT-SPI D-Bus interfaces an object can implement.
typedef enum {
  INTERFACE_ACCESSIBLE,
  INTERFACE_APPLICATION,
  INTERFACE_COMPONENT,
  INTERFACE_ACTION,
  INTERFACE_VALUE,
  INTERFACE_TEXT,
  INTERFACE_EDITABLE_TEXT,
  INTERFACE_SELECTION,
  INTERFACE_IMAGE,
  INTERFACE_SOCKET,
  INTERFACE_COUNT,
} Interface;

struct _FlAtspiObject {
  GObject parent_instance;

  // The application this object is in.
  FlAtspiApplication* application;

  // The accessible being exported.
  FlAtspiAccessible* accessible;

  GDBusConnection* connection;
  gchar* path;

  // The registration ID of each interface, or 0 if it is not exported.
  guint registration_ids[INTERFACE_COUNT];

  // The ID the registry gives the application.
  gint32 application_id;

  // The socket this window is embedded in, set by Socket.Embedded.
  gchar* parent_name;
  gchar* parent_path;
};

static void fl_atspi_object_dbus_accessible_iface_init(
    FlAtspiDBusAccessibleInterface* iface);
static void fl_atspi_object_dbus_application_iface_init(
    FlAtspiDBusApplicationInterface* iface);
static void fl_atspi_object_dbus_component_iface_init(
    FlAtspiDBusComponentInterface* iface);
static void fl_atspi_object_dbus_action_iface_init(
    FlAtspiDBusActionInterface* iface);
static void fl_atspi_object_dbus_value_iface_init(
    FlAtspiDBusValueInterface* iface);
static void fl_atspi_object_dbus_text_iface_init(
    FlAtspiDBusTextInterface* iface);
static void fl_atspi_object_dbus_editable_text_iface_init(
    FlAtspiDBusEditableTextInterface* iface);
static void fl_atspi_object_dbus_selection_iface_init(
    FlAtspiDBusSelectionInterface* iface);
static void fl_atspi_object_dbus_image_iface_init(
    FlAtspiDBusImageInterface* iface);
static void fl_atspi_object_dbus_socket_iface_init(
    FlAtspiDBusSocketInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlAtspiObject,
    fl_atspi_object,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(fl_atspi_dbus_accessible_get_type(),
                          fl_atspi_object_dbus_accessible_iface_init)
        G_IMPLEMENT_INTERFACE(fl_atspi_dbus_application_get_type(),
                              fl_atspi_object_dbus_application_iface_init)
            G_IMPLEMENT_INTERFACE(fl_atspi_dbus_component_get_type(),
                                  fl_atspi_object_dbus_component_iface_init)
                G_IMPLEMENT_INTERFACE(fl_atspi_dbus_action_get_type(),
                                      fl_atspi_object_dbus_action_iface_init)
                    G_IMPLEMENT_INTERFACE(fl_atspi_dbus_value_get_type(),
                                          fl_atspi_object_dbus_value_iface_init)
                        G_IMPLEMENT_INTERFACE(
                            fl_atspi_dbus_text_get_type(),
                            fl_atspi_object_dbus_text_iface_init)
                            G_IMPLEMENT_INTERFACE(
                                fl_atspi_dbus_editable_text_get_type(),
                                fl_atspi_object_dbus_editable_text_iface_init)
                                G_IMPLEMENT_INTERFACE(
                                    fl_atspi_dbus_selection_get_type(),
                                    fl_atspi_object_dbus_selection_iface_init)
                                    G_IMPLEMENT_INTERFACE(
                                        fl_atspi_dbus_image_get_type(),
                                        fl_atspi_object_dbus_image_iface_init)
                                        G_IMPLEMENT_INTERFACE(
                                            fl_atspi_dbus_socket_get_type(),
                                            fl_atspi_object_dbus_socket_iface_init))

static const gchar* get_locale() {
  const gchar* locale = setlocale(LC_MESSAGES, nullptr);
  return locale != nullptr ? locale : "";
}

// Returns the non-localized name of a role, e.g. "check box".
static const gchar* get_role_name(FlAtspiRole role) {
  switch (role) {
    case FL_ATSPI_ROLE_INVALID:
      return "invalid";
    case FL_ATSPI_ROLE_CHECK_BOX:
      return "check box";
    case FL_ATSPI_ROLE_DIALOG:
      return "dialog";
    case FL_ATSPI_ROLE_IMAGE:
      return "image";
    case FL_ATSPI_ROLE_LABEL:
      return "label";
    case FL_ATSPI_ROLE_PANEL:
      return "panel";
    case FL_ATSPI_ROLE_PASSWORD_TEXT:
      return "password text";
    case FL_ATSPI_ROLE_BUTTON:
      return "button";
    case FL_ATSPI_ROLE_RADIO_BUTTON:
      return "radio button";
    case FL_ATSPI_ROLE_SCROLL_PANE:
      return "scroll pane";
    case FL_ATSPI_ROLE_SLIDER:
      return "slider";
    case FL_ATSPI_ROLE_APPLICATION:
      return "application";
    case FL_ATSPI_ROLE_ENTRY:
      return "entry";
    case FL_ATSPI_ROLE_HEADING:
      return "heading";
    case FL_ATSPI_ROLE_LINK:
      return "link";
    case FL_ATSPI_ROLE_SWITCH:
      return "switch";
  }
  return "unknown";
}

// TRUE if |object| is a window, i.e. a child of the application.
static gboolean is_window(FlAtspiAccessible* object) {
  return FL_IS_ATSPI_APPLICATION(fl_atspi_accessible_get_parent(object));
}

// Gets the origin of the window containing |object| in |coord_type|
// coordinates, relative to the window content for parent coordinates.
static void get_origin(FlAtspiAccessible* object,
                       guint32 coord_type,
                       gint* x,
                       gint* y) {
  *x = *y = 0;
  if (coord_type > FL_ATSPI_COORD_TYPE_WINDOW) {
    return;
  }
  for (FlAtspiAccessible* o = object; o != nullptr;
       o = fl_atspi_accessible_get_parent(o)) {
    if (is_window(o)) {
      fl_atspi_accessible_get_origin(
          o, static_cast<FlAtspiCoordType>(coord_type), x, y);
      return;
    }
  }
}

// Returns the extents of |object| in |coord_type| coordinates.
static GdkRectangle get_extents(FlAtspiAccessible* object, guint32 coord_type) {
  GdkRectangle extents;
  fl_atspi_accessible_get_extents(object, &extents);
  gint origin_x, origin_y;
  get_origin(object, coord_type, &origin_x, &origin_y);
  extents.x += origin_x;
  extents.y += origin_y;
  return extents;
}

static GVariant* make_extents(const GdkRectangle* extents) {
  return g_variant_new("(iiii)", extents->x, extents->y, extents->width,
                       extents->height);
}

static GVariant* make_reference(FlAtspiObject* self,
                                FlAtspiAccessible* object) {
  return fl_atspi_application_make_reference(self->application, object);
}

static GDBusInterfaceInfo* get_interface_info(Interface interface) {
  switch (interface) {
    case INTERFACE_ACCESSIBLE:
      return fl_atspi_dbus_accessible_interface_info();
    case INTERFACE_APPLICATION:
      return fl_atspi_dbus_application_interface_info();
    case INTERFACE_COMPONENT:
      return fl_atspi_dbus_component_interface_info();
    case INTERFACE_ACTION:
      return fl_atspi_dbus_action_interface_info();
    case INTERFACE_VALUE:
      return fl_atspi_dbus_value_interface_info();
    case INTERFACE_TEXT:
      return fl_atspi_dbus_text_interface_info();
    case INTERFACE_EDITABLE_TEXT:
      return fl_atspi_dbus_editable_text_interface_info();
    case INTERFACE_SELECTION:
      return fl_atspi_dbus_selection_interface_info();
    case INTERFACE_IMAGE:
      return fl_atspi_dbus_image_interface_info();
    case INTERFACE_SOCKET:
      return fl_atspi_dbus_socket_interface_info();
    case INTERFACE_COUNT:
      break;
  }
  return nullptr;
}

// TRUE if the accessible currently implements |interface|.
static gboolean has_interface(FlAtspiObject* self, Interface interface) {
  FlAtspiAccessible* object = self->accessible;
  gboolean is_application = FL_IS_ATSPI_APPLICATION(object);
  switch (interface) {
    case INTERFACE_ACCESSIBLE:
      return TRUE;
    case INTERFACE_APPLICATION:
      return is_application;
    case INTERFACE_COMPONENT: {
      GdkRectangle extents;
      return !is_application &&
             fl_atspi_accessible_get_extents(object, &extents);
    }
    case INTERFACE_ACTION: {
      if (is_application) {
        return FALSE;
      }
      g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
      return actions->len > 0;
    }
    case INTERFACE_VALUE: {
      if (is_application) {
        return FALSE;
      }
      g_autoptr(FlAtspiValue) value = fl_atspi_accessible_get_value(object);
      return value != nullptr;
    }
    case INTERFACE_TEXT:
      return !is_application && fl_atspi_accessible_has_text(object);
    case INTERFACE_EDITABLE_TEXT:
      return !is_application && fl_atspi_accessible_is_editable_text(object);
    case INTERFACE_SELECTION:
      return !is_application &&
             fl_atspi_accessible_is_selection_container(object);
    case INTERFACE_IMAGE:
      return !is_application && fl_atspi_accessible_is_image(object);
    case INTERFACE_SOCKET:
      return !is_application && is_window(object);
    case INTERFACE_COUNT:
      break;
  }
  return FALSE;
}

static guint register_interface(FlAtspiObject* self, Interface interface) {
  g_autoptr(GError) error = nullptr;
  guint id = 0;
  switch (interface) {
    case INTERFACE_ACCESSIBLE:
      id = fl_atspi_dbus_accessible_register(
          FL_ATSPI_DBUS_ACCESSIBLE(self), self->connection, self->path, &error);
      break;
    case INTERFACE_APPLICATION:
      id = fl_atspi_dbus_application_register(FL_ATSPI_DBUS_APPLICATION(self),
                                              self->connection, self->path,
                                              &error);
      break;
    case INTERFACE_COMPONENT:
      id = fl_atspi_dbus_component_register(
          FL_ATSPI_DBUS_COMPONENT(self), self->connection, self->path, &error);
      break;
    case INTERFACE_ACTION:
      id = fl_atspi_dbus_action_register(FL_ATSPI_DBUS_ACTION(self),
                                         self->connection, self->path, &error);
      break;
    case INTERFACE_VALUE:
      id = fl_atspi_dbus_value_register(FL_ATSPI_DBUS_VALUE(self),
                                        self->connection, self->path, &error);
      break;
    case INTERFACE_TEXT:
      id = fl_atspi_dbus_text_register(FL_ATSPI_DBUS_TEXT(self),
                                       self->connection, self->path, &error);
      break;
    case INTERFACE_EDITABLE_TEXT:
      id = fl_atspi_dbus_editable_text_register(
          FL_ATSPI_DBUS_EDITABLE_TEXT(self), self->connection, self->path,
          &error);
      break;
    case INTERFACE_SELECTION:
      id = fl_atspi_dbus_selection_register(
          FL_ATSPI_DBUS_SELECTION(self), self->connection, self->path, &error);
      break;
    case INTERFACE_IMAGE:
      id = fl_atspi_dbus_image_register(FL_ATSPI_DBUS_IMAGE(self),
                                        self->connection, self->path, &error);
      break;
    case INTERFACE_SOCKET:
      id = fl_atspi_dbus_socket_register(FL_ATSPI_DBUS_SOCKET(self),
                                         self->connection, self->path, &error);
      break;
    case INTERFACE_COUNT:
      break;
  }
  if (id == 0) {
    g_warning("Failed to export %s: %s", self->path, error->message);
  }
  return id;
}

// Exports the interfaces the accessible implements and unexports those it no
// longer does.
static void update_interfaces(FlAtspiObject* self) {
  for (int i = 0; i < INTERFACE_COUNT; i++) {
    Interface interface = static_cast<Interface>(i);
    gboolean has = has_interface(self, interface);
    gboolean exported = self->registration_ids[i] != 0;
    if (has && !exported) {
      self->registration_ids[i] = register_interface(self, interface);
    } else if (!has && exported) {
      g_dbus_connection_unregister_object(self->connection,
                                          self->registration_ids[i]);
      self->registration_ids[i] = 0;
    }
  }
}

// org.a11y.atspi.Accessible

static gboolean accessible_handle_get_child_at_index(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation,
    gint32 index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_accessible_complete_get_child_at_index(
      dbus, invocation,
      make_reference(
          FL_ATSPI_OBJECT(dbus),
          index >= 0 ? fl_atspi_accessible_get_child(object, index) : nullptr));
  return TRUE;
}

static gboolean accessible_handle_get_children(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(so)"));
  gsize n_children = fl_atspi_accessible_get_n_children(object);
  for (gsize i = 0; i < n_children; i++) {
    g_variant_builder_add_value(
        &builder, make_reference(FL_ATSPI_OBJECT(dbus),
                                 fl_atspi_accessible_get_child(object, i)));
  }
  fl_atspi_dbus_accessible_complete_get_children(
      dbus, invocation, g_variant_builder_end(&builder));
  return TRUE;
}

static gboolean accessible_handle_get_index_in_parent(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiObject* self = FL_ATSPI_OBJECT(dbus);
  // A window is the only child of its socket.
  fl_atspi_dbus_accessible_complete_get_index_in_parent(
      dbus, invocation,
      self->parent_path != nullptr
          ? 0
          : fl_atspi_accessible_get_index_in_parent(self->accessible));
  return TRUE;
}

static gboolean accessible_handle_get_relation_set(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ua(so))"));
  g_autoptr(GPtrArray) relations = fl_atspi_accessible_get_relations(object);
  for (guint i = 0; i < relations->len; i++) {
    FlAtspiRelation* relation =
        static_cast<FlAtspiRelation*>(g_ptr_array_index(relations, i));
    GVariantBuilder targets;
    g_variant_builder_init(&targets, G_VARIANT_TYPE("a(so)"));
    for (guint j = 0; j < relation->targets->len; j++) {
      g_variant_builder_add_value(
          &targets, make_reference(FL_ATSPI_OBJECT(dbus),
                                   FL_ATSPI_ACCESSIBLE(g_ptr_array_index(
                                       relation->targets, j))));
    }
    g_variant_builder_add(&builder, "(u@a(so))",
                          static_cast<guint32>(relation->type),
                          g_variant_builder_end(&targets));
  }
  fl_atspi_dbus_accessible_complete_get_relation_set(
      dbus, invocation, g_variant_builder_end(&builder));
  return TRUE;
}

static gboolean accessible_handle_get_role(FlAtspiDBusAccessible* dbus,
                                           GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_accessible_complete_get_role(
      dbus, invocation,
      static_cast<guint32>(fl_atspi_accessible_get_role(object)));
  return TRUE;
}

static gboolean accessible_handle_get_role_name(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_accessible_complete_get_role_name(
      dbus, invocation, get_role_name(fl_atspi_accessible_get_role(object)));
  return TRUE;
}

static gboolean accessible_handle_get_localized_role_name(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_accessible_complete_get_localized_role_name(
      dbus, invocation, get_role_name(fl_atspi_accessible_get_role(object)));
  return TRUE;
}

static gboolean accessible_handle_get_state(FlAtspiDBusAccessible* dbus,
                                            GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  guint64 states = fl_atspi_accessible_get_states(object);
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("au"));
  g_variant_builder_add(&builder, "u", static_cast<guint32>(states));
  g_variant_builder_add(&builder, "u", static_cast<guint32>(states >> 32));
  fl_atspi_dbus_accessible_complete_get_state(dbus, invocation,
                                              g_variant_builder_end(&builder));
  return TRUE;
}

static gboolean accessible_handle_get_attributes(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));
  g_autoptr(GHashTable) attributes = fl_atspi_accessible_get_attributes(object);
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, attributes);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    g_variant_builder_add(&builder, "{ss}", key, value);
  }
  fl_atspi_dbus_accessible_complete_get_attributes(
      dbus, invocation, g_variant_builder_end(&builder));
  return TRUE;
}

static gboolean accessible_handle_get_application(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiObject* self = FL_ATSPI_OBJECT(dbus);
  fl_atspi_dbus_accessible_complete_get_application(
      dbus, invocation,
      make_reference(self, FL_ATSPI_ACCESSIBLE(self->application)));
  return TRUE;
}

static gboolean accessible_handle_get_interfaces(
    FlAtspiDBusAccessible* dbus,
    GDBusMethodInvocation* invocation) {
  FlAtspiObject* self = FL_ATSPI_OBJECT(dbus);
  // The client will use the interfaces it is told about, so export them.
  update_interfaces(self);
  g_autoptr(GPtrArray) names = g_ptr_array_new();
  for (int i = 0; i < INTERFACE_COUNT; i++) {
    if (self->registration_ids[i] != 0) {
      g_ptr_array_add(names,
                      get_interface_info(static_cast<Interface>(i))->name);
    }
  }
  g_ptr_array_add(names, nullptr);
  fl_atspi_dbus_accessible_complete_get_interfaces(
      dbus, invocation, reinterpret_cast<const gchar* const*>(names->pdata));
  return TRUE;
}

static gchar* accessible_get_name(FlAtspiDBusAccessible* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_name(object);
}

static gchar* accessible_get_description(FlAtspiDBusAccessible* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_description(object);
}

static GVariant* accessible_get_parent(FlAtspiDBusAccessible* dbus) {
  FlAtspiObject* self = FL_ATSPI_OBJECT(dbus);
  if (FL_IS_ATSPI_APPLICATION(self->accessible)) {
    return g_variant_new("(so)", "", kNullPath);
  }
  // A window is a child of the socket it is embedded in.
  if (self->parent_path != nullptr) {
    return g_variant_new("(so)", self->parent_name, self->parent_path);
  }
  return make_reference(self, fl_atspi_accessible_get_parent(self->accessible));
}

static gint32 accessible_get_child_count(FlAtspiDBusAccessible* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_n_children(object);
}

static gchar* accessible_get_locale(FlAtspiDBusAccessible* dbus) {
  return g_strdup(get_locale());
}

static gchar* accessible_get_accessible_id(FlAtspiDBusAccessible* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_accessible_id(object);
}

static gchar* accessible_get_help_text(FlAtspiDBusAccessible* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_help_text(object);
}

static void fl_atspi_object_dbus_accessible_iface_init(
    FlAtspiDBusAccessibleInterface* iface) {
  iface->handle_get_child_at_index = accessible_handle_get_child_at_index;
  iface->handle_get_children = accessible_handle_get_children;
  iface->handle_get_index_in_parent = accessible_handle_get_index_in_parent;
  iface->handle_get_relation_set = accessible_handle_get_relation_set;
  iface->handle_get_role = accessible_handle_get_role;
  iface->handle_get_role_name = accessible_handle_get_role_name;
  iface->handle_get_localized_role_name =
      accessible_handle_get_localized_role_name;
  iface->handle_get_state = accessible_handle_get_state;
  iface->handle_get_attributes = accessible_handle_get_attributes;
  iface->handle_get_application = accessible_handle_get_application;
  iface->handle_get_interfaces = accessible_handle_get_interfaces;
  iface->get_name = accessible_get_name;
  iface->get_description = accessible_get_description;
  iface->get_parent = accessible_get_parent;
  iface->get_child_count = accessible_get_child_count;
  iface->get_locale = accessible_get_locale;
  iface->get_accessible_id = accessible_get_accessible_id;
  iface->get_help_text = accessible_get_help_text;
}

// org.a11y.atspi.Application

static gboolean application_handle_get_locale(FlAtspiDBusApplication* dbus,
                                              GDBusMethodInvocation* invocation,
                                              guint32 lctype) {
  fl_atspi_dbus_application_complete_get_locale(dbus, invocation, get_locale());
  return TRUE;
}

static gboolean application_handle_get_application_bus_address(
    FlAtspiDBusApplication* dbus,
    GDBusMethodInvocation* invocation) {
  // Clients use the accessibility bus rather than a direct connection.
  fl_atspi_dbus_application_complete_get_application_bus_address(
      dbus, invocation, "");
  return TRUE;
}

static gchar* application_get_toolkit_name(FlAtspiDBusApplication* dbus) {
  return g_strdup("Flutter");
}

static gchar* application_get_version(FlAtspiDBusApplication* dbus) {
  return g_strdup("1.0");
}

static gchar* application_get_atspi_version(FlAtspiDBusApplication* dbus) {
  return g_strdup("2.1");
}

static guint32 application_get_interface_version(FlAtspiDBusApplication* dbus) {
  return 0;
}

static gint32 application_get_id(FlAtspiDBusApplication* dbus) {
  return FL_ATSPI_OBJECT(dbus)->application_id;
}

static void application_set_id(FlAtspiDBusApplication* dbus, gint32 value) {
  FL_ATSPI_OBJECT(dbus)->application_id = value;
}

static void fl_atspi_object_dbus_application_iface_init(
    FlAtspiDBusApplicationInterface* iface) {
  iface->handle_get_locale = application_handle_get_locale;
  iface->handle_get_application_bus_address =
      application_handle_get_application_bus_address;
  iface->get_toolkit_name = application_get_toolkit_name;
  iface->get_version = application_get_version;
  iface->get_toolkit_version = application_get_version;
  iface->get_atspi_version = application_get_atspi_version;
  iface->get_interface_version = application_get_interface_version;
  iface->get_id = application_get_id;
  iface->set_id = application_set_id;
}

// org.a11y.atspi.Component

// Converts the point |x|,|y| in |coord_type| coordinates to the coordinates
// used for |object|'s extents.
static void to_local_point(FlAtspiAccessible* object,
                           guint32 coord_type,
                           gint32* x,
                           gint32* y) {
  gint origin_x, origin_y;
  get_origin(object, coord_type, &origin_x, &origin_y);
  *x -= origin_x;
  *y -= origin_y;
}

static gboolean component_handle_contains(FlAtspiDBusComponent* dbus,
                                          GDBusMethodInvocation* invocation,
                                          gint32 x,
                                          gint32 y,
                                          guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  to_local_point(object, coord_type, &x, &y);
  GdkRectangle extents;
  fl_atspi_accessible_get_extents(object, &extents);
  fl_atspi_dbus_component_complete_contains(dbus, invocation,
                                            x >= extents.x && y >= extents.y &&
                                                x < extents.x + extents.width &&
                                                y < extents.y + extents.height);
  return TRUE;
}

static gboolean component_handle_get_accessible_at_point(
    FlAtspiDBusComponent* dbus,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  to_local_point(object, coord_type, &x, &y);
  fl_atspi_dbus_component_complete_get_accessible_at_point(
      dbus, invocation,
      make_reference(
          FL_ATSPI_OBJECT(dbus),
          fl_atspi_accessible_get_accessible_at_point(object, x, y)));
  return TRUE;
}

static gboolean component_handle_get_extents(FlAtspiDBusComponent* dbus,
                                             GDBusMethodInvocation* invocation,
                                             guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_component_complete_get_extents(dbus, invocation,
                                               make_extents(&extents));
  return TRUE;
}

static gboolean component_handle_get_position(FlAtspiDBusComponent* dbus,
                                              GDBusMethodInvocation* invocation,
                                              guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_component_complete_get_position(dbus, invocation, extents.x,
                                                extents.y);
  return TRUE;
}

static gboolean component_handle_get_size(FlAtspiDBusComponent* dbus,
                                          GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents;
  fl_atspi_accessible_get_extents(object, &extents);
  fl_atspi_dbus_component_complete_get_size(dbus, invocation, extents.width,
                                            extents.height);
  return TRUE;
}

static gboolean component_handle_get_layer(FlAtspiDBusComponent* dbus,
                                           GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_component_complete_get_layer(
      dbus, invocation,
      is_window(FL_ATSPI_OBJECT(dbus)->accessible) ? kLayerWindow
                                                   : kLayerWidget);
  return TRUE;
}

static gboolean component_handle_get_mdizorder(
    FlAtspiDBusComponent* dbus,
    GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_component_complete_get_mdizorder(dbus, invocation, 0);
  return TRUE;
}

static gboolean component_handle_grab_focus(FlAtspiDBusComponent* dbus,
                                            GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_component_complete_grab_focus(
      dbus, invocation, fl_atspi_accessible_grab_focus(object));
  return TRUE;
}

static gboolean component_handle_get_alpha(FlAtspiDBusComponent* dbus,
                                           GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_component_complete_get_alpha(dbus, invocation, 1.0);
  return TRUE;
}

static gboolean component_handle_set_extents(FlAtspiDBusComponent* dbus,
                                             GDBusMethodInvocation* invocation,
                                             gint32 x,
                                             gint32 y,
                                             gint32 width,
                                             gint32 height,
                                             guint32 coord_type) {
  fl_atspi_dbus_component_complete_set_extents(dbus, invocation, FALSE);
  return TRUE;
}

static gboolean component_handle_set_position(FlAtspiDBusComponent* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 x,
                                              gint32 y,
                                              guint32 coord_type) {
  fl_atspi_dbus_component_complete_set_position(dbus, invocation, FALSE);
  return TRUE;
}

static gboolean component_handle_set_size(FlAtspiDBusComponent* dbus,
                                          GDBusMethodInvocation* invocation,
                                          gint32 width,
                                          gint32 height) {
  fl_atspi_dbus_component_complete_set_size(dbus, invocation, FALSE);
  return TRUE;
}

static gboolean component_handle_scroll_to(FlAtspiDBusComponent* dbus,
                                           GDBusMethodInvocation* invocation,
                                           guint32 type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_component_complete_scroll_to(
      dbus, invocation, fl_atspi_accessible_scroll_to(object));
  return TRUE;
}

static gboolean component_handle_scroll_to_point(
    FlAtspiDBusComponent* dbus,
    GDBusMethodInvocation* invocation,
    guint32 coord_type,
    gint32 x,
    gint32 y) {
  fl_atspi_dbus_component_complete_scroll_to_point(dbus, invocation, FALSE);
  return TRUE;
}

static void fl_atspi_object_dbus_component_iface_init(
    FlAtspiDBusComponentInterface* iface) {
  iface->handle_contains = component_handle_contains;
  iface->handle_get_accessible_at_point =
      component_handle_get_accessible_at_point;
  iface->handle_get_extents = component_handle_get_extents;
  iface->handle_get_position = component_handle_get_position;
  iface->handle_get_size = component_handle_get_size;
  iface->handle_get_layer = component_handle_get_layer;
  iface->handle_get_mdizorder = component_handle_get_mdizorder;
  iface->handle_grab_focus = component_handle_grab_focus;
  iface->handle_get_alpha = component_handle_get_alpha;
  iface->handle_set_extents = component_handle_set_extents;
  iface->handle_set_position = component_handle_set_position;
  iface->handle_set_size = component_handle_set_size;
  iface->handle_scroll_to = component_handle_scroll_to;
  iface->handle_scroll_to_point = component_handle_scroll_to_point;
}

// org.a11y.atspi.Action

// Returns the action at |index| of |actions| or nullptr if out of range.
static FlAtspiAction* get_action(GPtrArray* actions, gint32 index) {
  return index >= 0 && static_cast<guint>(index) < actions->len
             ? static_cast<FlAtspiAction*>(g_ptr_array_index(actions, index))
             : nullptr;
}

static gboolean action_handle_get_description(FlAtspiDBusAction* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  FlAtspiAction* action = get_action(actions, index);
  fl_atspi_dbus_action_complete_get_description(
      dbus, invocation, action != nullptr ? action->description : "");
  return TRUE;
}

static gboolean action_handle_get_name(FlAtspiDBusAction* dbus,
                                       GDBusMethodInvocation* invocation,
                                       gint32 index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  FlAtspiAction* action = get_action(actions, index);
  fl_atspi_dbus_action_complete_get_name(dbus, invocation,
                                         action != nullptr ? action->name : "");
  return TRUE;
}

static gboolean action_handle_get_localized_name(
    FlAtspiDBusAction* dbus,
    GDBusMethodInvocation* invocation,
    gint32 index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  FlAtspiAction* action = get_action(actions, index);
  fl_atspi_dbus_action_complete_get_localized_name(
      dbus, invocation, action != nullptr ? action->name : "");
  return TRUE;
}

static gboolean action_handle_get_key_binding(FlAtspiDBusAction* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 index) {
  fl_atspi_dbus_action_complete_get_key_binding(dbus, invocation, "");
  return TRUE;
}

static gboolean action_handle_get_actions(FlAtspiDBusAction* dbus,
                                          GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sss)"));
  for (guint i = 0; i < actions->len; i++) {
    FlAtspiAction* action =
        static_cast<FlAtspiAction*>(g_ptr_array_index(actions, i));
    g_variant_builder_add(&builder, "(sss)", action->name, action->description,
                          "");
  }
  fl_atspi_dbus_action_complete_get_actions(dbus, invocation,
                                            g_variant_builder_end(&builder));
  return TRUE;
}

static gboolean action_handle_do_action(FlAtspiDBusAction* dbus,
                                        GDBusMethodInvocation* invocation,
                                        gint32 index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  fl_atspi_dbus_action_complete_do_action(
      dbus, invocation,
      get_action(actions, index) != nullptr &&
          fl_atspi_accessible_do_action(object, index));
  return TRUE;
}

static gint32 action_get_nactions(FlAtspiDBusAction* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GPtrArray) actions = fl_atspi_accessible_get_actions(object);
  return actions->len;
}

static void fl_atspi_object_dbus_action_iface_init(
    FlAtspiDBusActionInterface* iface) {
  iface->handle_get_description = action_handle_get_description;
  iface->handle_get_name = action_handle_get_name;
  iface->handle_get_localized_name = action_handle_get_localized_name;
  iface->handle_get_key_binding = action_handle_get_key_binding;
  iface->handle_get_actions = action_handle_get_actions;
  iface->handle_do_action = action_handle_do_action;
  iface->get_nactions = action_get_nactions;
}

// org.a11y.atspi.Value

static gdouble value_get_minimum_value(FlAtspiDBusValue* dbus) {
  g_autoptr(FlAtspiValue) value =
      fl_atspi_accessible_get_value(FL_ATSPI_OBJECT(dbus)->accessible);
  return value != nullptr ? value->minimum : 0;
}

static gdouble value_get_maximum_value(FlAtspiDBusValue* dbus) {
  g_autoptr(FlAtspiValue) value =
      fl_atspi_accessible_get_value(FL_ATSPI_OBJECT(dbus)->accessible);
  return value != nullptr ? value->maximum : 0;
}

static gdouble value_get_minimum_increment(FlAtspiDBusValue* dbus) {
  g_autoptr(FlAtspiValue) value =
      fl_atspi_accessible_get_value(FL_ATSPI_OBJECT(dbus)->accessible);
  return value != nullptr ? value->increment : 0;
}

static gdouble value_get_current_value(FlAtspiDBusValue* dbus) {
  g_autoptr(FlAtspiValue) value =
      fl_atspi_accessible_get_value(FL_ATSPI_OBJECT(dbus)->accessible);
  return value != nullptr ? value->current : 0;
}

static void value_set_current_value(FlAtspiDBusValue* dbus, gdouble value) {
  fl_atspi_accessible_set_value(FL_ATSPI_OBJECT(dbus)->accessible, value);
}

static gchar* value_get_text(FlAtspiDBusValue* dbus) {
  g_autoptr(FlAtspiValue) value =
      fl_atspi_accessible_get_value(FL_ATSPI_OBJECT(dbus)->accessible);
  return g_strdup(value != nullptr ? value->text : nullptr);
}

static void fl_atspi_object_dbus_value_iface_init(
    FlAtspiDBusValueInterface* iface) {
  iface->get_minimum_value = value_get_minimum_value;
  iface->get_maximum_value = value_get_maximum_value;
  iface->get_minimum_increment = value_get_minimum_increment;
  iface->get_current_value = value_get_current_value;
  iface->set_current_value = value_set_current_value;
  iface->get_text = value_get_text;
}

// org.a11y.atspi.Text

static void complete_text_range(FlAtspiDBusText* dbus,
                                GDBusMethodInvocation* invocation,
                                const gchar* text,
                                gint start,
                                gint end) {
  g_autofree gchar* substring = fl_atspi_text_substring(text, start, end);
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(sii)", substring, start, end));
}

static gboolean text_handle_get_string_at_offset(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    guint32 granularity) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  gint start, end;
  fl_atspi_text_get_string_at_offset(
      text, offset, static_cast<FlAtspiTextGranularity>(granularity), &start,
      &end);
  g_autofree gchar* substring = fl_atspi_text_substring(text, start, end);
  fl_atspi_dbus_text_complete_get_string_at_offset(dbus, invocation, substring,
                                                   start, end);
  return TRUE;
}

static gboolean text_handle_get_text(FlAtspiDBusText* dbus,
                                     GDBusMethodInvocation* invocation,
                                     gint32 start_offset,
                                     gint32 end_offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  g_autofree gchar* substring =
      fl_atspi_text_substring(text, start_offset, end_offset);
  fl_atspi_dbus_text_complete_get_text(dbus, invocation, substring);
  return TRUE;
}

static gboolean text_handle_set_caret_offset(FlAtspiDBusText* dbus,
                                             GDBusMethodInvocation* invocation,
                                             gint32 offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  offset = CLAMP(offset, 0, g_utf8_strlen(text, -1));
  fl_atspi_dbus_text_complete_set_caret_offset(
      dbus, invocation,
      fl_atspi_accessible_set_selection(object, offset, offset));
  return TRUE;
}

// Handles GetTextBeforeOffset, GetTextAtOffset and GetTextAfterOffset.
static void get_text_at_offset(FlAtspiDBusText* dbus,
                               GDBusMethodInvocation* invocation,
                               gint32 offset,
                               guint32 type,
                               FlAtspiTextPosition position) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  if (type > FL_ATSPI_TEXT_BOUNDARY_LINE_END) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
        "Invalid arguments to %s",
        g_dbus_method_invocation_get_method_name(invocation));
    return;
  }
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  gint start, end;
  fl_atspi_text_get_text_at_offset(text, offset,
                                   static_cast<FlAtspiTextBoundary>(type),
                                   position, &start, &end);
  complete_text_range(dbus, invocation, text, start, end);
}

static gboolean text_handle_get_text_before_offset(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    guint32 type) {
  get_text_at_offset(dbus, invocation, offset, type,
                     FL_ATSPI_TEXT_POSITION_BEFORE);
  return TRUE;
}

static gboolean text_handle_get_text_at_offset(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    guint32 type) {
  get_text_at_offset(dbus, invocation, offset, type, FL_ATSPI_TEXT_POSITION_AT);
  return TRUE;
}

static gboolean text_handle_get_text_after_offset(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    guint32 type) {
  get_text_at_offset(dbus, invocation, offset, type,
                     FL_ATSPI_TEXT_POSITION_AFTER);
  return TRUE;
}

static gboolean text_handle_get_character_at_offset(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  gunichar character = 0;
  if (offset >= 0 && offset < g_utf8_strlen(text, -1)) {
    character = g_utf8_get_char(g_utf8_offset_to_pointer(text, offset));
  }
  fl_atspi_dbus_text_complete_get_character_at_offset(
      dbus, invocation, static_cast<gint32>(character));
  return TRUE;
}

static gboolean text_handle_get_attribute_value(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    const gchar* attribute_name) {
  fl_atspi_dbus_text_complete_get_attribute_value(dbus, invocation, "");
  return TRUE;
}

static gboolean text_handle_get_attributes(FlAtspiDBusText* dbus,
                                           GDBusMethodInvocation* invocation,
                                           gint32 offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  fl_atspi_dbus_text_complete_get_attributes(dbus, invocation,
                                             g_variant_new("a{ss}", nullptr), 0,
                                             g_utf8_strlen(text, -1));
  return TRUE;
}

static gboolean text_handle_get_default_attributes(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_text_complete_get_default_attributes(
      dbus, invocation, g_variant_new("a{ss}", nullptr));
  return TRUE;
}

static gboolean text_handle_get_character_extents(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 offset,
    guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  // Flutter doesn't describe the layout of the text.
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_text_complete_get_character_extents(
      dbus, invocation, extents.x, extents.y, extents.width, extents.height);
  return TRUE;
}

static gboolean text_handle_get_offset_at_point(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    guint32 coord_type) {
  fl_atspi_dbus_text_complete_get_offset_at_point(dbus, invocation, -1);
  return TRUE;
}

static gboolean text_handle_get_nselections(FlAtspiDBusText* dbus,
                                            GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  gint start, end;
  gboolean has_selection =
      fl_atspi_accessible_get_selection(object, &start, &end);
  fl_atspi_dbus_text_complete_get_nselections(dbus, invocation,
                                              has_selection ? 1 : 0);
  return TRUE;
}

static gboolean text_handle_get_selection(FlAtspiDBusText* dbus,
                                          GDBusMethodInvocation* invocation,
                                          gint32 selection_num) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  gint start = 0, end = 0;
  gboolean valid = selection_num == 0 &&
                   fl_atspi_accessible_get_selection(object, &start, &end);
  fl_atspi_dbus_text_complete_get_selection(dbus, invocation, valid ? start : 0,
                                            valid ? end : 0);
  return TRUE;
}

static gboolean text_handle_add_selection(FlAtspiDBusText* dbus,
                                          GDBusMethodInvocation* invocation,
                                          gint32 start_offset,
                                          gint32 end_offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  gint start, end;
  // Only one selection is supported.
  gboolean has_selection =
      fl_atspi_accessible_get_selection(object, &start, &end);
  fl_atspi_dbus_text_complete_add_selection(
      dbus, invocation,
      !has_selection &&
          fl_atspi_accessible_set_selection(object, start_offset, end_offset));
  return TRUE;
}

static gboolean text_handle_remove_selection(FlAtspiDBusText* dbus,
                                             GDBusMethodInvocation* invocation,
                                             gint32 selection_num) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  gint start, end;
  gboolean has_selection =
      fl_atspi_accessible_get_selection(object, &start, &end);
  gint caret = fl_atspi_accessible_get_caret_offset(object);
  fl_atspi_dbus_text_complete_remove_selection(
      dbus, invocation,
      selection_num == 0 && has_selection &&
          fl_atspi_accessible_set_selection(object, caret, caret));
  return TRUE;
}

static gboolean text_handle_set_selection(FlAtspiDBusText* dbus,
                                          GDBusMethodInvocation* invocation,
                                          gint32 selection_num,
                                          gint32 start_offset,
                                          gint32 end_offset) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_text_complete_set_selection(
      dbus, invocation,
      selection_num == 0 &&
          fl_atspi_accessible_set_selection(object, start_offset, end_offset));
  return TRUE;
}

static gboolean text_handle_get_range_extents(FlAtspiDBusText* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 start_offset,
                                              gint32 end_offset,
                                              guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  // Flutter doesn't describe the layout of the text.
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_text_complete_get_range_extents(
      dbus, invocation, extents.x, extents.y, extents.width, extents.height);
  return TRUE;
}

static gboolean text_handle_get_bounded_ranges(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    gint32 width,
    gint32 height,
    guint32 coord_type,
    guint32 x_clip_type,
    guint32 y_clip_type) {
  fl_atspi_dbus_text_complete_get_bounded_ranges(
      dbus, invocation, g_variant_new("a(iisv)", nullptr));
  return TRUE;
}

static gboolean text_handle_get_attribute_run(FlAtspiDBusText* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 offset,
                                              gboolean include_defaults) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  fl_atspi_dbus_text_complete_get_attribute_run(dbus, invocation,
                                                g_variant_new("a{ss}", nullptr),
                                                0, g_utf8_strlen(text, -1));
  return TRUE;
}

static gboolean text_handle_get_default_attribute_set(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_text_complete_get_default_attribute_set(
      dbus, invocation, g_variant_new("a{ss}", nullptr));
  return TRUE;
}

static gboolean text_handle_scroll_substring_to(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 start_offset,
    gint32 end_offset,
    guint32 type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_text_complete_scroll_substring_to(
      dbus, invocation, fl_atspi_accessible_scroll_to(object));
  return TRUE;
}

static gboolean text_handle_scroll_substring_to_point(
    FlAtspiDBusText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 start_offset,
    gint32 end_offset,
    guint32 coord_type,
    gint32 x,
    gint32 y) {
  fl_atspi_dbus_text_complete_scroll_substring_to_point(dbus, invocation,
                                                        FALSE);
  return TRUE;
}

static gint32 text_get_character_count(FlAtspiDBusText* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  return g_utf8_strlen(text, -1);
}

static gint32 text_get_caret_offset(FlAtspiDBusText* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_caret_offset(object);
}

static void fl_atspi_object_dbus_text_iface_init(
    FlAtspiDBusTextInterface* iface) {
  iface->handle_get_string_at_offset = text_handle_get_string_at_offset;
  iface->handle_get_text = text_handle_get_text;
  iface->handle_set_caret_offset = text_handle_set_caret_offset;
  iface->handle_get_text_before_offset = text_handle_get_text_before_offset;
  iface->handle_get_text_at_offset = text_handle_get_text_at_offset;
  iface->handle_get_text_after_offset = text_handle_get_text_after_offset;
  iface->handle_get_character_at_offset = text_handle_get_character_at_offset;
  iface->handle_get_attribute_value = text_handle_get_attribute_value;
  iface->handle_get_attributes = text_handle_get_attributes;
  iface->handle_get_default_attributes = text_handle_get_default_attributes;
  iface->handle_get_character_extents = text_handle_get_character_extents;
  iface->handle_get_offset_at_point = text_handle_get_offset_at_point;
  iface->handle_get_nselections = text_handle_get_nselections;
  iface->handle_get_selection = text_handle_get_selection;
  iface->handle_add_selection = text_handle_add_selection;
  iface->handle_remove_selection = text_handle_remove_selection;
  iface->handle_set_selection = text_handle_set_selection;
  iface->handle_get_range_extents = text_handle_get_range_extents;
  iface->handle_get_bounded_ranges = text_handle_get_bounded_ranges;
  iface->handle_get_attribute_run = text_handle_get_attribute_run;
  iface->handle_get_default_attribute_set =
      text_handle_get_default_attribute_set;
  iface->handle_scroll_substring_to = text_handle_scroll_substring_to;
  iface->handle_scroll_substring_to_point =
      text_handle_scroll_substring_to_point;
  iface->get_character_count = text_get_character_count;
  iface->get_caret_offset = text_get_caret_offset;
}

// org.a11y.atspi.EditableText

// Replaces the characters of the text of |object| from |start| to |end| with
// |replacement|.
static gboolean replace_text(FlAtspiAccessible* object,
                             gint start,
                             gint end,
                             const gchar* replacement) {
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  gint count = g_utf8_strlen(text, -1);
  if (end < 0 || end > count) {
    end = count;
  }
  start = CLAMP(start, 0, end);
  const gchar* start_pointer = g_utf8_offset_to_pointer(text, start);
  const gchar* end_pointer = g_utf8_offset_to_pointer(text, end);
  g_autoptr(GString) new_text = g_string_new_len(text, start_pointer - text);
  g_string_append(new_text, replacement);
  g_string_append(new_text, end_pointer);
  return fl_atspi_accessible_set_text_contents(object, new_text->str);
}

// Selects the text from |start| to |end| and performs |action| on it.
static gboolean do_clipboard_action(FlAtspiAccessible* object,
                                    gint start,
                                    gint end,
                                    FlAtspiClipboardAction action) {
  return fl_atspi_accessible_set_selection(object, start, end) &&
         fl_atspi_accessible_do_clipboard_action(object, action);
}

static gboolean editable_text_handle_set_text_contents(
    FlAtspiDBusEditableText* dbus,
    GDBusMethodInvocation* invocation,
    const gchar* new_contents) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_editable_text_complete_set_text_contents(
      dbus, invocation,
      fl_atspi_accessible_set_text_contents(object, new_contents));
  return TRUE;
}

static gboolean editable_text_handle_insert_text(
    FlAtspiDBusEditableText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 position,
    const gchar* text,
    gint32 length) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autofree gchar* inserted =
      fl_atspi_text_substring(text, 0, length >= 0 ? length : -1);
  fl_atspi_dbus_editable_text_complete_insert_text(
      dbus, invocation, replace_text(object, position, position, inserted));
  return TRUE;
}

static gboolean editable_text_handle_copy_text(
    FlAtspiDBusEditableText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 start_pos,
    gint32 end_pos) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  do_clipboard_action(object, start_pos, end_pos,
                      FL_ATSPI_CLIPBOARD_ACTION_COPY);
  fl_atspi_dbus_editable_text_complete_copy_text(dbus, invocation);
  return TRUE;
}

static gboolean editable_text_handle_cut_text(FlAtspiDBusEditableText* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 start_pos,
                                              gint32 end_pos) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_editable_text_complete_cut_text(
      dbus, invocation,
      do_clipboard_action(object, start_pos, end_pos,
                          FL_ATSPI_CLIPBOARD_ACTION_CUT));
  return TRUE;
}

static gboolean editable_text_handle_delete_text(
    FlAtspiDBusEditableText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 start_pos,
    gint32 end_pos) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_editable_text_complete_delete_text(
      dbus, invocation, replace_text(object, start_pos, end_pos, ""));
  return TRUE;
}

static gboolean editable_text_handle_paste_text(
    FlAtspiDBusEditableText* dbus,
    GDBusMethodInvocation* invocation,
    gint32 position) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_editable_text_complete_paste_text(
      dbus, invocation,
      do_clipboard_action(object, position, position,
                          FL_ATSPI_CLIPBOARD_ACTION_PASTE));
  return TRUE;
}

static void fl_atspi_object_dbus_editable_text_iface_init(
    FlAtspiDBusEditableTextInterface* iface) {
  iface->handle_set_text_contents = editable_text_handle_set_text_contents;
  iface->handle_insert_text = editable_text_handle_insert_text;
  iface->handle_copy_text = editable_text_handle_copy_text;
  iface->handle_cut_text = editable_text_handle_cut_text;
  iface->handle_delete_text = editable_text_handle_delete_text;
  iface->handle_paste_text = editable_text_handle_paste_text;
}

// org.a11y.atspi.Selection

static gboolean selection_handle_get_selected_child(
    FlAtspiDBusSelection* dbus,
    GDBusMethodInvocation* invocation,
    gint32 selected_child_index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GArray) selected =
      fl_atspi_accessible_get_selected_children(object);
  FlAtspiAccessible* child = nullptr;
  if (selected_child_index >= 0 &&
      static_cast<guint>(selected_child_index) < selected->len) {
    child = fl_atspi_accessible_get_child(
        object, g_array_index(selected, guint, selected_child_index));
  }
  fl_atspi_dbus_selection_complete_get_selected_child(
      dbus, invocation, make_reference(FL_ATSPI_OBJECT(dbus), child));
  return TRUE;
}

static gboolean selection_handle_select_child(FlAtspiDBusSelection* dbus,
                                              GDBusMethodInvocation* invocation,
                                              gint32 child_index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  fl_atspi_dbus_selection_complete_select_child(
      dbus, invocation,
      child_index >= 0 &&
          fl_atspi_accessible_select_child(object, child_index));
  return TRUE;
}

static gboolean selection_handle_deselect_selected_child(
    FlAtspiDBusSelection* dbus,
    GDBusMethodInvocation* invocation,
    gint32 selected_child_index) {
  // Selected items, e.g. tabs, can't be deselected without selecting another.
  fl_atspi_dbus_selection_complete_deselect_selected_child(dbus, invocation,
                                                           FALSE);
  return TRUE;
}

static gboolean selection_handle_is_child_selected(
    FlAtspiDBusSelection* dbus,
    GDBusMethodInvocation* invocation,
    gint32 child_index) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GArray) selected =
      fl_atspi_accessible_get_selected_children(object);
  gboolean is_selected = FALSE;
  for (guint i = 0; i < selected->len; i++) {
    if (child_index >= 0 &&
        g_array_index(selected, guint, i) == static_cast<guint>(child_index)) {
      is_selected = TRUE;
    }
  }
  fl_atspi_dbus_selection_complete_is_child_selected(dbus, invocation,
                                                     is_selected);
  return TRUE;
}

static gboolean selection_handle_select_all(FlAtspiDBusSelection* dbus,
                                            GDBusMethodInvocation* invocation) {
  // Flutter has no way to change the selection of several items at once.
  fl_atspi_dbus_selection_complete_select_all(dbus, invocation, FALSE);
  return TRUE;
}

static gboolean selection_handle_clear_selection(
    FlAtspiDBusSelection* dbus,
    GDBusMethodInvocation* invocation) {
  fl_atspi_dbus_selection_complete_clear_selection(dbus, invocation, FALSE);
  return TRUE;
}

static gboolean selection_handle_deselect_child(
    FlAtspiDBusSelection* dbus,
    GDBusMethodInvocation* invocation,
    gint32 child_index) {
  fl_atspi_dbus_selection_complete_deselect_child(dbus, invocation, FALSE);
  return TRUE;
}

static gint32 selection_get_nselected_children(FlAtspiDBusSelection* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  g_autoptr(GArray) selected =
      fl_atspi_accessible_get_selected_children(object);
  return selected->len;
}

static void fl_atspi_object_dbus_selection_iface_init(
    FlAtspiDBusSelectionInterface* iface) {
  iface->handle_get_selected_child = selection_handle_get_selected_child;
  iface->handle_select_child = selection_handle_select_child;
  iface->handle_deselect_selected_child =
      selection_handle_deselect_selected_child;
  iface->handle_is_child_selected = selection_handle_is_child_selected;
  iface->handle_select_all = selection_handle_select_all;
  iface->handle_clear_selection = selection_handle_clear_selection;
  iface->handle_deselect_child = selection_handle_deselect_child;
  iface->get_nselected_children = selection_get_nselected_children;
}

// org.a11y.atspi.Image

static gboolean image_handle_get_image_extents(
    FlAtspiDBusImage* dbus,
    GDBusMethodInvocation* invocation,
    guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_image_complete_get_image_extents(dbus, invocation,
                                                 make_extents(&extents));
  return TRUE;
}

static gboolean image_handle_get_image_position(
    FlAtspiDBusImage* dbus,
    GDBusMethodInvocation* invocation,
    guint32 coord_type) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents = get_extents(object, coord_type);
  fl_atspi_dbus_image_complete_get_image_position(dbus, invocation, extents.x,
                                                  extents.y);
  return TRUE;
}

static gboolean image_handle_get_image_size(FlAtspiDBusImage* dbus,
                                            GDBusMethodInvocation* invocation) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  GdkRectangle extents;
  fl_atspi_accessible_get_extents(object, &extents);
  fl_atspi_dbus_image_complete_get_image_size(dbus, invocation, extents.width,
                                              extents.height);
  return TRUE;
}

static gchar* image_get_image_description(FlAtspiDBusImage* dbus) {
  FlAtspiAccessible* object = FL_ATSPI_OBJECT(dbus)->accessible;
  return fl_atspi_accessible_get_name(object);
}

static gchar* image_get_image_locale(FlAtspiDBusImage* dbus) {
  return g_strdup(get_locale());
}

static void fl_atspi_object_dbus_image_iface_init(
    FlAtspiDBusImageInterface* iface) {
  iface->handle_get_image_extents = image_handle_get_image_extents;
  iface->handle_get_image_position = image_handle_get_image_position;
  iface->handle_get_image_size = image_handle_get_image_size;
  iface->get_image_description = image_get_image_description;
  iface->get_image_locale = image_get_image_locale;
}

// org.a11y.atspi.Socket

static gboolean socket_handle_embedded(FlAtspiDBusSocket* dbus,
                                       GDBusMethodInvocation* invocation,
                                       const gchar* path) {
  // Sent by at-spi2-atk when this window is embedded into an AtkSocket.
  FlAtspiObject* self = FL_ATSPI_OBJECT(dbus);
  const gchar* sender = g_dbus_method_invocation_get_sender(invocation);
  g_free(self->parent_name);
  self->parent_name = g_strdup(sender != nullptr ? sender : "");
  g_free(self->parent_path);
  self->parent_path = g_strdup(path);
  fl_atspi_dbus_socket_complete_embedded(dbus, invocation);
  return TRUE;
}

static void fl_atspi_object_dbus_socket_iface_init(
    FlAtspiDBusSocketInterface* iface) {
  iface->handle_embedded = socket_handle_embedded;
}

static void fl_atspi_object_dispose(GObject* object) {
  FlAtspiObject* self = FL_ATSPI_OBJECT(object);

  for (int i = 0; i < INTERFACE_COUNT; i++) {
    if (self->registration_ids[i] != 0) {
      g_dbus_connection_unregister_object(self->connection,
                                          self->registration_ids[i]);
      self->registration_ids[i] = 0;
    }
  }
  g_clear_object(&self->connection);
  g_clear_pointer(&self->path, g_free);
  g_clear_pointer(&self->parent_name, g_free);
  g_clear_pointer(&self->parent_path, g_free);

  G_OBJECT_CLASS(fl_atspi_object_parent_class)->dispose(object);
}

static void fl_atspi_object_class_init(FlAtspiObjectClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_atspi_object_dispose;
}

static void fl_atspi_object_init(FlAtspiObject* self) {}

FlAtspiObject* fl_atspi_object_new(FlAtspiApplication* application,
                                   FlAtspiAccessible* accessible,
                                   GDBusConnection* connection,
                                   const gchar* path) {
  FlAtspiObject* self =
      FL_ATSPI_OBJECT(g_object_new(fl_atspi_object_get_type(), nullptr));
  self->application = application;
  self->accessible = accessible;
  self->connection = G_DBUS_CONNECTION(g_object_ref(connection));
  self->path = g_strdup(path);
  update_interfaces(self);
  return self;
}

const gchar* fl_atspi_object_get_path(FlAtspiObject* self) {
  g_return_val_if_fail(FL_IS_ATSPI_OBJECT(self), nullptr);
  return self->path;
}
