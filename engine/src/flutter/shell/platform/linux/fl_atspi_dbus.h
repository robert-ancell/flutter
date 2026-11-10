// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_DBUS_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_DBUS_H_

#include <gio/gio.h>

G_BEGIN_DECLS

// Bindings for the AT-SPI D-Bus interfaces, in the style of gdbus-codegen.

/**
 * fl_atspi_dbus_bus_call_get_address:
 * @connection: a #GDBusConnection.
 * @bus_name: (allow-none): the bus name of the object, or %NULL on a peer
 * connection.
 * @object_path: the object path of the object.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the call is complete.
 * @user_data: (closure): user data to pass to @callback.
 *
 * Asynchronously calls the GetAddress() method on org.a11y.Bus.
 */
void fl_atspi_dbus_bus_call_get_address(GDBusConnection* connection,
                                        const gchar* bus_name,
                                        const gchar* object_path,
                                        GCancellable* cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data);

/**
 * fl_atspi_dbus_bus_call_get_address_finish:
 * @connection: a #GDBusConnection.
 * @result: the #GAsyncResult passed to the callback.
 * @address: (out): location to write the address argument, free with g_free().
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Completes a call started with fl_atspi_dbus_bus_call_get_address().
 *
 * Returns: %TRUE on success.
 */
gboolean fl_atspi_dbus_bus_call_get_address_finish(GDBusConnection* connection,
                                                   GAsyncResult* result,
                                                   gchar** address,
                                                   GError** error);

/**
 * fl_atspi_dbus_registry_call_get_registered_events:
 * @connection: a #GDBusConnection.
 * @bus_name: (allow-none): the bus name of the object, or %NULL on a peer
 * connection.
 * @object_path: the object path of the object.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the call is complete.
 * @user_data: (closure): user data to pass to @callback.
 *
 * Asynchronously calls the GetRegisteredEvents() method on
 * org.a11y.atspi.Registry.
 */
void fl_atspi_dbus_registry_call_get_registered_events(
    GDBusConnection* connection,
    const gchar* bus_name,
    const gchar* object_path,
    GCancellable* cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

/**
 * fl_atspi_dbus_registry_call_get_registered_events_finish:
 * @connection: a #GDBusConnection.
 * @result: the #GAsyncResult passed to the callback.
 * @events: (out): location to write the events argument, free with
 * g_variant_unref().
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Completes a call started with
 * fl_atspi_dbus_registry_call_get_registered_events().
 *
 * Returns: %TRUE on success.
 */
gboolean fl_atspi_dbus_registry_call_get_registered_events_finish(
    GDBusConnection* connection,
    GAsyncResult* result,
    GVariant** events,
    GError** error);

/**
 * FlAtspiDBusRegistryEventListenerRegisteredHandler:
 * @bus: the bus argument.
 * @event: the event argument.
 * @properties: the properties argument.
 * @user_data: user data passed to
 * fl_atspi_dbus_registry_subscribe_event_listener_registered().
 *
 * Function called when the EventListenerRegistered signal is received.
 */
typedef void (*FlAtspiDBusRegistryEventListenerRegisteredHandler)(
    const gchar* bus,
    const gchar* event,
    const gchar* const* properties,
    gpointer user_data);

/**
 * fl_atspi_dbus_registry_subscribe_event_listener_registered:
 * @connection: a #GDBusConnection.
 * @sender: (allow-none): the bus name to receive signals from, or %NULL on a
 * peer connection.
 * @object_path: the object path to receive signals from.
 * @handler: function to call when the signal is received.
 * @user_data: (closure): user data to pass to @handler.
 *
 * Subscribes to the EventListenerRegistered signal on org.a11y.atspi.Registry.
 *
 * Returns: a subscription ID to pass to g_dbus_connection_signal_unsubscribe().
 */
guint fl_atspi_dbus_registry_subscribe_event_listener_registered(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    FlAtspiDBusRegistryEventListenerRegisteredHandler handler,
    gpointer user_data);

/**
 * FlAtspiDBusRegistryEventListenerDeregisteredHandler:
 * @bus: the bus argument.
 * @event: the event argument.
 * @user_data: user data passed to
 * fl_atspi_dbus_registry_subscribe_event_listener_deregistered().
 *
 * Function called when the EventListenerDeregistered signal is received.
 */
typedef void (*FlAtspiDBusRegistryEventListenerDeregisteredHandler)(
    const gchar* bus,
    const gchar* event,
    gpointer user_data);

/**
 * fl_atspi_dbus_registry_subscribe_event_listener_deregistered:
 * @connection: a #GDBusConnection.
 * @sender: (allow-none): the bus name to receive signals from, or %NULL on a
 * peer connection.
 * @object_path: the object path to receive signals from.
 * @handler: function to call when the signal is received.
 * @user_data: (closure): user data to pass to @handler.
 *
 * Subscribes to the EventListenerDeregistered signal on
 * org.a11y.atspi.Registry.
 *
 * Returns: a subscription ID to pass to g_dbus_connection_signal_unsubscribe().
 */
guint fl_atspi_dbus_registry_subscribe_event_listener_deregistered(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    FlAtspiDBusRegistryEventListenerDeregisteredHandler handler,
    gpointer user_data);

G_DECLARE_INTERFACE(FlAtspiDBusAccessible,
                    fl_atspi_dbus_accessible,
                    FL,
                    ATSPI_DBUS_ACCESSIBLE,
                    GObject);

/**
 * FlAtspiDBusAccessible:
 *
 * #FlAtspiDBusAccessible is implemented by objects that handle the
 * org.a11y.atspi.Accessible D-Bus interface. Method
 * handlers must complete the call, e.g. with
 * fl_atspi_dbus_accessible_complete_*(), and return %TRUE. Methods and
 * properties without an implementation return an error.
 */

struct _FlAtspiDBusAccessibleInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_child_at_index)(FlAtspiDBusAccessible* self,
                                        GDBusMethodInvocation* invocation,
                                        gint32 index);

  gboolean (*handle_get_children)(FlAtspiDBusAccessible* self,
                                  GDBusMethodInvocation* invocation);

  gboolean (*handle_get_index_in_parent)(FlAtspiDBusAccessible* self,
                                         GDBusMethodInvocation* invocation);

  gboolean (*handle_get_relation_set)(FlAtspiDBusAccessible* self,
                                      GDBusMethodInvocation* invocation);

  gboolean (*handle_get_role)(FlAtspiDBusAccessible* self,
                              GDBusMethodInvocation* invocation);

  gboolean (*handle_get_role_name)(FlAtspiDBusAccessible* self,
                                   GDBusMethodInvocation* invocation);

  gboolean (*handle_get_localized_role_name)(FlAtspiDBusAccessible* self,
                                             GDBusMethodInvocation* invocation);

  gboolean (*handle_get_state)(FlAtspiDBusAccessible* self,
                               GDBusMethodInvocation* invocation);

  gboolean (*handle_get_attributes)(FlAtspiDBusAccessible* self,
                                    GDBusMethodInvocation* invocation);

  gboolean (*handle_get_application)(FlAtspiDBusAccessible* self,
                                     GDBusMethodInvocation* invocation);

  gboolean (*handle_get_interfaces)(FlAtspiDBusAccessible* self,
                                    GDBusMethodInvocation* invocation);

  gchar* (*get_name)(FlAtspiDBusAccessible* self);

  gchar* (*get_description)(FlAtspiDBusAccessible* self);

  GVariant* (*get_parent)(FlAtspiDBusAccessible* self);

  gint32 (*get_child_count)(FlAtspiDBusAccessible* self);

  gchar* (*get_locale)(FlAtspiDBusAccessible* self);

  gchar* (*get_accessible_id)(FlAtspiDBusAccessible* self);

  gchar* (*get_help_text)(FlAtspiDBusAccessible* self);
};

/**
 * fl_atspi_dbus_accessible_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Accessible.
 */
GDBusInterfaceInfo* fl_atspi_dbus_accessible_interface_info();

/**
 * fl_atspi_dbus_accessible_register:
 * @self: an #FlAtspiDBusAccessible.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_accessible_register(FlAtspiDBusAccessible* self,
                                        GDBusConnection* connection,
                                        const gchar* object_path,
                                        GError** error);

/**
 * fl_atspi_dbus_accessible_complete_get_child_at_index:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @child: the child argument.
 *
 * Completes handling the GetChildAtIndex() method.
 */
void fl_atspi_dbus_accessible_complete_get_child_at_index(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* child);

/**
 * fl_atspi_dbus_accessible_complete_get_children:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @children: the children argument.
 *
 * Completes handling the GetChildren() method.
 */
void fl_atspi_dbus_accessible_complete_get_children(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* children);

/**
 * fl_atspi_dbus_accessible_complete_get_index_in_parent:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @index: the index argument.
 *
 * Completes handling the GetIndexInParent() method.
 */
void fl_atspi_dbus_accessible_complete_get_index_in_parent(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    gint32 index);

/**
 * fl_atspi_dbus_accessible_complete_get_relation_set:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @relations: the relations argument.
 *
 * Completes handling the GetRelationSet() method.
 */
void fl_atspi_dbus_accessible_complete_get_relation_set(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* relations);

/**
 * fl_atspi_dbus_accessible_complete_get_role:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @role: the role argument.
 *
 * Completes handling the GetRole() method.
 */
void fl_atspi_dbus_accessible_complete_get_role(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    guint32 role);

/**
 * fl_atspi_dbus_accessible_complete_get_role_name:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @name: the name argument.
 *
 * Completes handling the GetRoleName() method.
 */
void fl_atspi_dbus_accessible_complete_get_role_name(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* name);

/**
 * fl_atspi_dbus_accessible_complete_get_localized_role_name:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @name: the name argument.
 *
 * Completes handling the GetLocalizedRoleName() method.
 */
void fl_atspi_dbus_accessible_complete_get_localized_role_name(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* name);

/**
 * fl_atspi_dbus_accessible_complete_get_state:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @state: the state argument.
 *
 * Completes handling the GetState() method.
 */
void fl_atspi_dbus_accessible_complete_get_state(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* state);

/**
 * fl_atspi_dbus_accessible_complete_get_attributes:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @attributes: the attributes argument.
 *
 * Completes handling the GetAttributes() method.
 */
void fl_atspi_dbus_accessible_complete_get_attributes(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes);

/**
 * fl_atspi_dbus_accessible_complete_get_application:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @application: the application argument.
 *
 * Completes handling the GetApplication() method.
 */
void fl_atspi_dbus_accessible_complete_get_application(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* application);

/**
 * fl_atspi_dbus_accessible_complete_get_interfaces:
 * @self: an #FlAtspiDBusAccessible.
 * @invocation: (transfer full): the invocation being handled.
 * @interfaces: the interfaces argument.
 *
 * Completes handling the GetInterfaces() method.
 */
void fl_atspi_dbus_accessible_complete_get_interfaces(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* const* interfaces);

G_DECLARE_INTERFACE(FlAtspiDBusApplication,
                    fl_atspi_dbus_application,
                    FL,
                    ATSPI_DBUS_APPLICATION,
                    GObject);

/**
 * FlAtspiDBusApplication:
 *
 * #FlAtspiDBusApplication is implemented by objects that handle the
 * org.a11y.atspi.Application D-Bus interface.
 * Method handlers must complete the call, e.g. with
 * fl_atspi_dbus_application_complete_*(), and return %TRUE. Methods and
 * properties without an implementation return an error.
 */

struct _FlAtspiDBusApplicationInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_locale)(FlAtspiDBusApplication* self,
                                GDBusMethodInvocation* invocation,
                                guint32 lctype);

  gboolean (*handle_get_application_bus_address)(
      FlAtspiDBusApplication* self,
      GDBusMethodInvocation* invocation);

  gchar* (*get_toolkit_name)(FlAtspiDBusApplication* self);

  gchar* (*get_version)(FlAtspiDBusApplication* self);

  gchar* (*get_toolkit_version)(FlAtspiDBusApplication* self);

  gchar* (*get_atspi_version)(FlAtspiDBusApplication* self);

  guint32 (*get_interface_version)(FlAtspiDBusApplication* self);

  gint32 (*get_id)(FlAtspiDBusApplication* self);

  void (*set_id)(FlAtspiDBusApplication* self, gint32 value);
};

/**
 * fl_atspi_dbus_application_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Application.
 */
GDBusInterfaceInfo* fl_atspi_dbus_application_interface_info();

/**
 * fl_atspi_dbus_application_register:
 * @self: an #FlAtspiDBusApplication.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_application_register(FlAtspiDBusApplication* self,
                                         GDBusConnection* connection,
                                         const gchar* object_path,
                                         GError** error);

/**
 * fl_atspi_dbus_application_complete_get_locale:
 * @self: an #FlAtspiDBusApplication.
 * @invocation: (transfer full): the invocation being handled.
 * @locale: the locale argument.
 *
 * Completes handling the GetLocale() method.
 */
void fl_atspi_dbus_application_complete_get_locale(
    FlAtspiDBusApplication* self,
    GDBusMethodInvocation* invocation,
    const gchar* locale);

/**
 * fl_atspi_dbus_application_complete_get_application_bus_address:
 * @self: an #FlAtspiDBusApplication.
 * @invocation: (transfer full): the invocation being handled.
 * @address: the address argument.
 *
 * Completes handling the GetApplicationBusAddress() method.
 */
void fl_atspi_dbus_application_complete_get_application_bus_address(
    FlAtspiDBusApplication* self,
    GDBusMethodInvocation* invocation,
    const gchar* address);

G_DECLARE_INTERFACE(FlAtspiDBusComponent,
                    fl_atspi_dbus_component,
                    FL,
                    ATSPI_DBUS_COMPONENT,
                    GObject);

/**
 * FlAtspiDBusComponent:
 *
 * #FlAtspiDBusComponent is implemented by objects that handle the
 * org.a11y.atspi.Component D-Bus interface. Method
 * handlers must complete the call, e.g. with
 * fl_atspi_dbus_component_complete_*(), and return %TRUE. Methods and
 * properties without an implementation return an error.
 */

struct _FlAtspiDBusComponentInterface {
  GTypeInterface g_iface;

  gboolean (*handle_contains)(FlAtspiDBusComponent* self,
                              GDBusMethodInvocation* invocation,
                              gint32 x,
                              gint32 y,
                              guint32 coord_type);

  gboolean (*handle_get_accessible_at_point)(FlAtspiDBusComponent* self,
                                             GDBusMethodInvocation* invocation,
                                             gint32 x,
                                             gint32 y,
                                             guint32 coord_type);

  gboolean (*handle_get_extents)(FlAtspiDBusComponent* self,
                                 GDBusMethodInvocation* invocation,
                                 guint32 coord_type);

  gboolean (*handle_get_position)(FlAtspiDBusComponent* self,
                                  GDBusMethodInvocation* invocation,
                                  guint32 coord_type);

  gboolean (*handle_get_size)(FlAtspiDBusComponent* self,
                              GDBusMethodInvocation* invocation);

  gboolean (*handle_get_layer)(FlAtspiDBusComponent* self,
                               GDBusMethodInvocation* invocation);

  gboolean (*handle_get_mdizorder)(FlAtspiDBusComponent* self,
                                   GDBusMethodInvocation* invocation);

  gboolean (*handle_grab_focus)(FlAtspiDBusComponent* self,
                                GDBusMethodInvocation* invocation);

  gboolean (*handle_get_alpha)(FlAtspiDBusComponent* self,
                               GDBusMethodInvocation* invocation);

  gboolean (*handle_set_extents)(FlAtspiDBusComponent* self,
                                 GDBusMethodInvocation* invocation,
                                 gint32 x,
                                 gint32 y,
                                 gint32 width,
                                 gint32 height,
                                 guint32 coord_type);

  gboolean (*handle_set_position)(FlAtspiDBusComponent* self,
                                  GDBusMethodInvocation* invocation,
                                  gint32 x,
                                  gint32 y,
                                  guint32 coord_type);

  gboolean (*handle_set_size)(FlAtspiDBusComponent* self,
                              GDBusMethodInvocation* invocation,
                              gint32 width,
                              gint32 height);

  gboolean (*handle_scroll_to)(FlAtspiDBusComponent* self,
                               GDBusMethodInvocation* invocation,
                               guint32 type);

  gboolean (*handle_scroll_to_point)(FlAtspiDBusComponent* self,
                                     GDBusMethodInvocation* invocation,
                                     guint32 coord_type,
                                     gint32 x,
                                     gint32 y);
};

/**
 * fl_atspi_dbus_component_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Component.
 */
GDBusInterfaceInfo* fl_atspi_dbus_component_interface_info();

/**
 * fl_atspi_dbus_component_register:
 * @self: an #FlAtspiDBusComponent.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_component_register(FlAtspiDBusComponent* self,
                                       GDBusConnection* connection,
                                       const gchar* object_path,
                                       GError** error);

/**
 * fl_atspi_dbus_component_complete_contains:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @contains: the contains argument.
 *
 * Completes handling the Contains() method.
 */
void fl_atspi_dbus_component_complete_contains(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean contains);

/**
 * fl_atspi_dbus_component_complete_get_accessible_at_point:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @accessible: the accessible argument.
 *
 * Completes handling the GetAccessibleAtPoint() method.
 */
void fl_atspi_dbus_component_complete_get_accessible_at_point(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    GVariant* accessible);

/**
 * fl_atspi_dbus_component_complete_get_extents:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @extents: the extents argument.
 *
 * Completes handling the GetExtents() method.
 */
void fl_atspi_dbus_component_complete_get_extents(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    GVariant* extents);

/**
 * fl_atspi_dbus_component_complete_get_position:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @x: the x argument.
 * @y: the y argument.
 *
 * Completes handling the GetPosition() method.
 */
void fl_atspi_dbus_component_complete_get_position(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y);

/**
 * fl_atspi_dbus_component_complete_get_size:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @width: the width argument.
 * @height: the height argument.
 *
 * Completes handling the GetSize() method.
 */
void fl_atspi_dbus_component_complete_get_size(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint32 width,
    gint32 height);

/**
 * fl_atspi_dbus_component_complete_get_layer:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @layer: the layer argument.
 *
 * Completes handling the GetLayer() method.
 */
void fl_atspi_dbus_component_complete_get_layer(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    guint32 layer);

/**
 * fl_atspi_dbus_component_complete_get_mdizorder:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @z_order: the z_order argument.
 *
 * Completes handling the GetMDIZOrder() method.
 */
void fl_atspi_dbus_component_complete_get_mdizorder(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint16 z_order);

/**
 * fl_atspi_dbus_component_complete_grab_focus:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the GrabFocus() method.
 */
void fl_atspi_dbus_component_complete_grab_focus(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_component_complete_get_alpha:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @alpha: the alpha argument.
 *
 * Completes handling the GetAlpha() method.
 */
void fl_atspi_dbus_component_complete_get_alpha(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gdouble alpha);

/**
 * fl_atspi_dbus_component_complete_set_extents:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetExtents() method.
 */
void fl_atspi_dbus_component_complete_set_extents(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_component_complete_set_position:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetPosition() method.
 */
void fl_atspi_dbus_component_complete_set_position(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_component_complete_set_size:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetSize() method.
 */
void fl_atspi_dbus_component_complete_set_size(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_component_complete_scroll_to:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the ScrollTo() method.
 */
void fl_atspi_dbus_component_complete_scroll_to(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_component_complete_scroll_to_point:
 * @self: an #FlAtspiDBusComponent.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the ScrollToPoint() method.
 */
void fl_atspi_dbus_component_complete_scroll_to_point(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

G_DECLARE_INTERFACE(FlAtspiDBusAction,
                    fl_atspi_dbus_action,
                    FL,
                    ATSPI_DBUS_ACTION,
                    GObject);

/**
 * FlAtspiDBusAction:
 *
 * #FlAtspiDBusAction is implemented by objects that handle the
 * org.a11y.atspi.Action D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_action_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusActionInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_description)(FlAtspiDBusAction* self,
                                     GDBusMethodInvocation* invocation,
                                     gint32 index);

  gboolean (*handle_get_name)(FlAtspiDBusAction* self,
                              GDBusMethodInvocation* invocation,
                              gint32 index);

  gboolean (*handle_get_localized_name)(FlAtspiDBusAction* self,
                                        GDBusMethodInvocation* invocation,
                                        gint32 index);

  gboolean (*handle_get_key_binding)(FlAtspiDBusAction* self,
                                     GDBusMethodInvocation* invocation,
                                     gint32 index);

  gboolean (*handle_get_actions)(FlAtspiDBusAction* self,
                                 GDBusMethodInvocation* invocation);

  gboolean (*handle_do_action)(FlAtspiDBusAction* self,
                               GDBusMethodInvocation* invocation,
                               gint32 index);

  gint32 (*get_nactions)(FlAtspiDBusAction* self);
};

/**
 * fl_atspi_dbus_action_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Action.
 */
GDBusInterfaceInfo* fl_atspi_dbus_action_interface_info();

/**
 * fl_atspi_dbus_action_register:
 * @self: an #FlAtspiDBusAction.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_action_register(FlAtspiDBusAction* self,
                                    GDBusConnection* connection,
                                    const gchar* object_path,
                                    GError** error);

/**
 * fl_atspi_dbus_action_complete_get_description:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @description: the description argument.
 *
 * Completes handling the GetDescription() method.
 */
void fl_atspi_dbus_action_complete_get_description(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* description);

/**
 * fl_atspi_dbus_action_complete_get_name:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @name: the name argument.
 *
 * Completes handling the GetName() method.
 */
void fl_atspi_dbus_action_complete_get_name(FlAtspiDBusAction* self,
                                            GDBusMethodInvocation* invocation,
                                            const gchar* name);

/**
 * fl_atspi_dbus_action_complete_get_localized_name:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @name: the name argument.
 *
 * Completes handling the GetLocalizedName() method.
 */
void fl_atspi_dbus_action_complete_get_localized_name(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* name);

/**
 * fl_atspi_dbus_action_complete_get_key_binding:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @key_binding: the key_binding argument.
 *
 * Completes handling the GetKeyBinding() method.
 */
void fl_atspi_dbus_action_complete_get_key_binding(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* key_binding);

/**
 * fl_atspi_dbus_action_complete_get_actions:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @actions: the actions argument.
 *
 * Completes handling the GetActions() method.
 */
void fl_atspi_dbus_action_complete_get_actions(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    GVariant* actions);

/**
 * fl_atspi_dbus_action_complete_do_action:
 * @self: an #FlAtspiDBusAction.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the DoAction() method.
 */
void fl_atspi_dbus_action_complete_do_action(FlAtspiDBusAction* self,
                                             GDBusMethodInvocation* invocation,
                                             gboolean result);

G_DECLARE_INTERFACE(FlAtspiDBusValue,
                    fl_atspi_dbus_value,
                    FL,
                    ATSPI_DBUS_VALUE,
                    GObject);

/**
 * FlAtspiDBusValue:
 *
 * #FlAtspiDBusValue is implemented by objects that handle the
 * org.a11y.atspi.Value D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_value_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusValueInterface {
  GTypeInterface g_iface;

  gdouble (*get_minimum_value)(FlAtspiDBusValue* self);

  gdouble (*get_maximum_value)(FlAtspiDBusValue* self);

  gdouble (*get_minimum_increment)(FlAtspiDBusValue* self);

  gdouble (*get_current_value)(FlAtspiDBusValue* self);

  void (*set_current_value)(FlAtspiDBusValue* self, gdouble value);

  gchar* (*get_text)(FlAtspiDBusValue* self);
};

/**
 * fl_atspi_dbus_value_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Value.
 */
GDBusInterfaceInfo* fl_atspi_dbus_value_interface_info();

/**
 * fl_atspi_dbus_value_register:
 * @self: an #FlAtspiDBusValue.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_value_register(FlAtspiDBusValue* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error);

G_DECLARE_INTERFACE(FlAtspiDBusText,
                    fl_atspi_dbus_text,
                    FL,
                    ATSPI_DBUS_TEXT,
                    GObject);

/**
 * FlAtspiDBusText:
 *
 * #FlAtspiDBusText is implemented by objects that handle the
 * org.a11y.atspi.Text D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_text_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusTextInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_string_at_offset)(FlAtspiDBusText* self,
                                          GDBusMethodInvocation* invocation,
                                          gint32 offset,
                                          guint32 granularity);

  gboolean (*handle_get_text)(FlAtspiDBusText* self,
                              GDBusMethodInvocation* invocation,
                              gint32 start_offset,
                              gint32 end_offset);

  gboolean (*handle_set_caret_offset)(FlAtspiDBusText* self,
                                      GDBusMethodInvocation* invocation,
                                      gint32 offset);

  gboolean (*handle_get_text_before_offset)(FlAtspiDBusText* self,
                                            GDBusMethodInvocation* invocation,
                                            gint32 offset,
                                            guint32 type);

  gboolean (*handle_get_text_at_offset)(FlAtspiDBusText* self,
                                        GDBusMethodInvocation* invocation,
                                        gint32 offset,
                                        guint32 type);

  gboolean (*handle_get_text_after_offset)(FlAtspiDBusText* self,
                                           GDBusMethodInvocation* invocation,
                                           gint32 offset,
                                           guint32 type);

  gboolean (*handle_get_character_at_offset)(FlAtspiDBusText* self,
                                             GDBusMethodInvocation* invocation,
                                             gint32 offset);

  gboolean (*handle_get_attribute_value)(FlAtspiDBusText* self,
                                         GDBusMethodInvocation* invocation,
                                         gint32 offset,
                                         const gchar* attribute_name);

  gboolean (*handle_get_attributes)(FlAtspiDBusText* self,
                                    GDBusMethodInvocation* invocation,
                                    gint32 offset);

  gboolean (*handle_get_default_attributes)(FlAtspiDBusText* self,
                                            GDBusMethodInvocation* invocation);

  gboolean (*handle_get_character_extents)(FlAtspiDBusText* self,
                                           GDBusMethodInvocation* invocation,
                                           gint32 offset,
                                           guint32 coord_type);

  gboolean (*handle_get_offset_at_point)(FlAtspiDBusText* self,
                                         GDBusMethodInvocation* invocation,
                                         gint32 x,
                                         gint32 y,
                                         guint32 coord_type);

  gboolean (*handle_get_nselections)(FlAtspiDBusText* self,
                                     GDBusMethodInvocation* invocation);

  gboolean (*handle_get_selection)(FlAtspiDBusText* self,
                                   GDBusMethodInvocation* invocation,
                                   gint32 selection_num);

  gboolean (*handle_add_selection)(FlAtspiDBusText* self,
                                   GDBusMethodInvocation* invocation,
                                   gint32 start_offset,
                                   gint32 end_offset);

  gboolean (*handle_remove_selection)(FlAtspiDBusText* self,
                                      GDBusMethodInvocation* invocation,
                                      gint32 selection_num);

  gboolean (*handle_set_selection)(FlAtspiDBusText* self,
                                   GDBusMethodInvocation* invocation,
                                   gint32 selection_num,
                                   gint32 start_offset,
                                   gint32 end_offset);

  gboolean (*handle_get_range_extents)(FlAtspiDBusText* self,
                                       GDBusMethodInvocation* invocation,
                                       gint32 start_offset,
                                       gint32 end_offset,
                                       guint32 coord_type);

  gboolean (*handle_get_bounded_ranges)(FlAtspiDBusText* self,
                                        GDBusMethodInvocation* invocation,
                                        gint32 x,
                                        gint32 y,
                                        gint32 width,
                                        gint32 height,
                                        guint32 coord_type,
                                        guint32 x_clip_type,
                                        guint32 y_clip_type);

  gboolean (*handle_get_attribute_run)(FlAtspiDBusText* self,
                                       GDBusMethodInvocation* invocation,
                                       gint32 offset,
                                       gboolean include_defaults);

  gboolean (*handle_get_default_attribute_set)(
      FlAtspiDBusText* self,
      GDBusMethodInvocation* invocation);

  gboolean (*handle_scroll_substring_to)(FlAtspiDBusText* self,
                                         GDBusMethodInvocation* invocation,
                                         gint32 start_offset,
                                         gint32 end_offset,
                                         guint32 type);

  gboolean (*handle_scroll_substring_to_point)(
      FlAtspiDBusText* self,
      GDBusMethodInvocation* invocation,
      gint32 start_offset,
      gint32 end_offset,
      guint32 coord_type,
      gint32 x,
      gint32 y);

  gint32 (*get_character_count)(FlAtspiDBusText* self);

  gint32 (*get_caret_offset)(FlAtspiDBusText* self);
};

/**
 * fl_atspi_dbus_text_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Text.
 */
GDBusInterfaceInfo* fl_atspi_dbus_text_interface_info();

/**
 * fl_atspi_dbus_text_register:
 * @self: an #FlAtspiDBusText.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_text_register(FlAtspiDBusText* self,
                                  GDBusConnection* connection,
                                  const gchar* object_path,
                                  GError** error);

/**
 * fl_atspi_dbus_text_complete_get_string_at_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @text: the text argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetStringAtOffset() method.
 */
void fl_atspi_dbus_text_complete_get_string_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_text:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @text: the text argument.
 *
 * Completes handling the GetText() method.
 */
void fl_atspi_dbus_text_complete_get_text(FlAtspiDBusText* self,
                                          GDBusMethodInvocation* invocation,
                                          const gchar* text);

/**
 * fl_atspi_dbus_text_complete_set_caret_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetCaretOffset() method.
 */
void fl_atspi_dbus_text_complete_set_caret_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_text_complete_get_text_before_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @text: the text argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetTextBeforeOffset() method.
 */
void fl_atspi_dbus_text_complete_get_text_before_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_text_at_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @text: the text argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetTextAtOffset() method.
 */
void fl_atspi_dbus_text_complete_get_text_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_text_after_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @text: the text argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetTextAfterOffset() method.
 */
void fl_atspi_dbus_text_complete_get_text_after_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_character_at_offset:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @character: the character argument.
 *
 * Completes handling the GetCharacterAtOffset() method.
 */
void fl_atspi_dbus_text_complete_get_character_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 character);

/**
 * fl_atspi_dbus_text_complete_get_attribute_value:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @value: the value argument.
 *
 * Completes handling the GetAttributeValue() method.
 */
void fl_atspi_dbus_text_complete_get_attribute_value(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* value);

/**
 * fl_atspi_dbus_text_complete_get_attributes:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @attributes: the attributes argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetAttributes() method.
 */
void fl_atspi_dbus_text_complete_get_attributes(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_default_attributes:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @attributes: the attributes argument.
 *
 * Completes handling the GetDefaultAttributes() method.
 */
void fl_atspi_dbus_text_complete_get_default_attributes(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes);

/**
 * fl_atspi_dbus_text_complete_get_character_extents:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @x: the x argument.
 * @y: the y argument.
 * @width: the width argument.
 * @height: the height argument.
 *
 * Completes handling the GetCharacterExtents() method.
 */
void fl_atspi_dbus_text_complete_get_character_extents(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    gint32 width,
    gint32 height);

/**
 * fl_atspi_dbus_text_complete_get_offset_at_point:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @offset: the offset argument.
 *
 * Completes handling the GetOffsetAtPoint() method.
 */
void fl_atspi_dbus_text_complete_get_offset_at_point(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 offset);

/**
 * fl_atspi_dbus_text_complete_get_nselections:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @n_selections: the n_selections argument.
 *
 * Completes handling the GetNSelections() method.
 */
void fl_atspi_dbus_text_complete_get_nselections(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 n_selections);

/**
 * fl_atspi_dbus_text_complete_get_selection:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetSelection() method.
 */
void fl_atspi_dbus_text_complete_get_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_add_selection:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the AddSelection() method.
 */
void fl_atspi_dbus_text_complete_add_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_text_complete_remove_selection:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the RemoveSelection() method.
 */
void fl_atspi_dbus_text_complete_remove_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_text_complete_set_selection:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetSelection() method.
 */
void fl_atspi_dbus_text_complete_set_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_text_complete_get_range_extents:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @x: the x argument.
 * @y: the y argument.
 * @width: the width argument.
 * @height: the height argument.
 *
 * Completes handling the GetRangeExtents() method.
 */
void fl_atspi_dbus_text_complete_get_range_extents(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    gint32 width,
    gint32 height);

/**
 * fl_atspi_dbus_text_complete_get_bounded_ranges:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @ranges: the ranges argument.
 *
 * Completes handling the GetBoundedRanges() method.
 */
void fl_atspi_dbus_text_complete_get_bounded_ranges(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* ranges);

/**
 * fl_atspi_dbus_text_complete_get_attribute_run:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @attributes: the attributes argument.
 * @start_offset: the start_offset argument.
 * @end_offset: the end_offset argument.
 *
 * Completes handling the GetAttributeRun() method.
 */
void fl_atspi_dbus_text_complete_get_attribute_run(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes,
    gint32 start_offset,
    gint32 end_offset);

/**
 * fl_atspi_dbus_text_complete_get_default_attribute_set:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @attributes: the attributes argument.
 *
 * Completes handling the GetDefaultAttributeSet() method.
 */
void fl_atspi_dbus_text_complete_get_default_attribute_set(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes);

/**
 * fl_atspi_dbus_text_complete_scroll_substring_to:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the ScrollSubstringTo() method.
 */
void fl_atspi_dbus_text_complete_scroll_substring_to(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_text_complete_scroll_substring_to_point:
 * @self: an #FlAtspiDBusText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the ScrollSubstringToPoint() method.
 */
void fl_atspi_dbus_text_complete_scroll_substring_to_point(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

G_DECLARE_INTERFACE(FlAtspiDBusEditableText,
                    fl_atspi_dbus_editable_text,
                    FL,
                    ATSPI_DBUS_EDITABLE_TEXT,
                    GObject);

/**
 * FlAtspiDBusEditableText:
 *
 * #FlAtspiDBusEditableText is implemented by objects that handle the
 * org.a11y.atspi.EditableText D-Bus interface.
 * Method handlers must complete the call, e.g. with
 * fl_atspi_dbus_editable_text_complete_*(), and return %TRUE. Methods and
 * properties without an implementation return an error.
 */

struct _FlAtspiDBusEditableTextInterface {
  GTypeInterface g_iface;

  gboolean (*handle_set_text_contents)(FlAtspiDBusEditableText* self,
                                       GDBusMethodInvocation* invocation,
                                       const gchar* new_contents);

  gboolean (*handle_insert_text)(FlAtspiDBusEditableText* self,
                                 GDBusMethodInvocation* invocation,
                                 gint32 position,
                                 const gchar* text,
                                 gint32 length);

  gboolean (*handle_copy_text)(FlAtspiDBusEditableText* self,
                               GDBusMethodInvocation* invocation,
                               gint32 start_pos,
                               gint32 end_pos);

  gboolean (*handle_cut_text)(FlAtspiDBusEditableText* self,
                              GDBusMethodInvocation* invocation,
                              gint32 start_pos,
                              gint32 end_pos);

  gboolean (*handle_delete_text)(FlAtspiDBusEditableText* self,
                                 GDBusMethodInvocation* invocation,
                                 gint32 start_pos,
                                 gint32 end_pos);

  gboolean (*handle_paste_text)(FlAtspiDBusEditableText* self,
                                GDBusMethodInvocation* invocation,
                                gint32 position);
};

/**
 * fl_atspi_dbus_editable_text_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.EditableText.
 */
GDBusInterfaceInfo* fl_atspi_dbus_editable_text_interface_info();

/**
 * fl_atspi_dbus_editable_text_register:
 * @self: an #FlAtspiDBusEditableText.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_editable_text_register(FlAtspiDBusEditableText* self,
                                           GDBusConnection* connection,
                                           const gchar* object_path,
                                           GError** error);

/**
 * fl_atspi_dbus_editable_text_complete_set_text_contents:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SetTextContents() method.
 */
void fl_atspi_dbus_editable_text_complete_set_text_contents(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_editable_text_complete_insert_text:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the InsertText() method.
 */
void fl_atspi_dbus_editable_text_complete_insert_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_editable_text_complete_copy_text:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 *
 * Completes handling the CopyText() method.
 */
void fl_atspi_dbus_editable_text_complete_copy_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation);

/**
 * fl_atspi_dbus_editable_text_complete_cut_text:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the CutText() method.
 */
void fl_atspi_dbus_editable_text_complete_cut_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_editable_text_complete_delete_text:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the DeleteText() method.
 */
void fl_atspi_dbus_editable_text_complete_delete_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_editable_text_complete_paste_text:
 * @self: an #FlAtspiDBusEditableText.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the PasteText() method.
 */
void fl_atspi_dbus_editable_text_complete_paste_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

G_DECLARE_INTERFACE(FlAtspiDBusSelection,
                    fl_atspi_dbus_selection,
                    FL,
                    ATSPI_DBUS_SELECTION,
                    GObject);

/**
 * FlAtspiDBusSelection:
 *
 * #FlAtspiDBusSelection is implemented by objects that handle the
 * org.a11y.atspi.Selection D-Bus interface. Method
 * handlers must complete the call, e.g. with
 * fl_atspi_dbus_selection_complete_*(), and return %TRUE. Methods and
 * properties without an implementation return an error.
 */

struct _FlAtspiDBusSelectionInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_selected_child)(FlAtspiDBusSelection* self,
                                        GDBusMethodInvocation* invocation,
                                        gint32 selected_child_index);

  gboolean (*handle_select_child)(FlAtspiDBusSelection* self,
                                  GDBusMethodInvocation* invocation,
                                  gint32 child_index);

  gboolean (*handle_deselect_selected_child)(FlAtspiDBusSelection* self,
                                             GDBusMethodInvocation* invocation,
                                             gint32 selected_child_index);

  gboolean (*handle_is_child_selected)(FlAtspiDBusSelection* self,
                                       GDBusMethodInvocation* invocation,
                                       gint32 child_index);

  gboolean (*handle_select_all)(FlAtspiDBusSelection* self,
                                GDBusMethodInvocation* invocation);

  gboolean (*handle_clear_selection)(FlAtspiDBusSelection* self,
                                     GDBusMethodInvocation* invocation);

  gboolean (*handle_deselect_child)(FlAtspiDBusSelection* self,
                                    GDBusMethodInvocation* invocation,
                                    gint32 child_index);

  gint32 (*get_nselected_children)(FlAtspiDBusSelection* self);
};

/**
 * fl_atspi_dbus_selection_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Selection.
 */
GDBusInterfaceInfo* fl_atspi_dbus_selection_interface_info();

/**
 * fl_atspi_dbus_selection_register:
 * @self: an #FlAtspiDBusSelection.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_selection_register(FlAtspiDBusSelection* self,
                                       GDBusConnection* connection,
                                       const gchar* object_path,
                                       GError** error);

/**
 * fl_atspi_dbus_selection_complete_get_selected_child:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @child: the child argument.
 *
 * Completes handling the GetSelectedChild() method.
 */
void fl_atspi_dbus_selection_complete_get_selected_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    GVariant* child);

/**
 * fl_atspi_dbus_selection_complete_select_child:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SelectChild() method.
 */
void fl_atspi_dbus_selection_complete_select_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_selection_complete_deselect_selected_child:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the DeselectSelectedChild() method.
 */
void fl_atspi_dbus_selection_complete_deselect_selected_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_selection_complete_is_child_selected:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the IsChildSelected() method.
 */
void fl_atspi_dbus_selection_complete_is_child_selected(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_selection_complete_select_all:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the SelectAll() method.
 */
void fl_atspi_dbus_selection_complete_select_all(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_selection_complete_clear_selection:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the ClearSelection() method.
 */
void fl_atspi_dbus_selection_complete_clear_selection(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

/**
 * fl_atspi_dbus_selection_complete_deselect_child:
 * @self: an #FlAtspiDBusSelection.
 * @invocation: (transfer full): the invocation being handled.
 * @result: the result argument.
 *
 * Completes handling the DeselectChild() method.
 */
void fl_atspi_dbus_selection_complete_deselect_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result);

G_DECLARE_INTERFACE(FlAtspiDBusImage,
                    fl_atspi_dbus_image,
                    FL,
                    ATSPI_DBUS_IMAGE,
                    GObject);

/**
 * FlAtspiDBusImage:
 *
 * #FlAtspiDBusImage is implemented by objects that handle the
 * org.a11y.atspi.Image D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_image_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusImageInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_image_extents)(FlAtspiDBusImage* self,
                                       GDBusMethodInvocation* invocation,
                                       guint32 coord_type);

  gboolean (*handle_get_image_position)(FlAtspiDBusImage* self,
                                        GDBusMethodInvocation* invocation,
                                        guint32 coord_type);

  gboolean (*handle_get_image_size)(FlAtspiDBusImage* self,
                                    GDBusMethodInvocation* invocation);

  gchar* (*get_image_description)(FlAtspiDBusImage* self);

  gchar* (*get_image_locale)(FlAtspiDBusImage* self);
};

/**
 * fl_atspi_dbus_image_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Image.
 */
GDBusInterfaceInfo* fl_atspi_dbus_image_interface_info();

/**
 * fl_atspi_dbus_image_register:
 * @self: an #FlAtspiDBusImage.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_image_register(FlAtspiDBusImage* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error);

/**
 * fl_atspi_dbus_image_complete_get_image_extents:
 * @self: an #FlAtspiDBusImage.
 * @invocation: (transfer full): the invocation being handled.
 * @extents: the extents argument.
 *
 * Completes handling the GetImageExtents() method.
 */
void fl_atspi_dbus_image_complete_get_image_extents(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    GVariant* extents);

/**
 * fl_atspi_dbus_image_complete_get_image_position:
 * @self: an #FlAtspiDBusImage.
 * @invocation: (transfer full): the invocation being handled.
 * @x: the x argument.
 * @y: the y argument.
 *
 * Completes handling the GetImagePosition() method.
 */
void fl_atspi_dbus_image_complete_get_image_position(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y);

/**
 * fl_atspi_dbus_image_complete_get_image_size:
 * @self: an #FlAtspiDBusImage.
 * @invocation: (transfer full): the invocation being handled.
 * @width: the width argument.
 * @height: the height argument.
 *
 * Completes handling the GetImageSize() method.
 */
void fl_atspi_dbus_image_complete_get_image_size(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    gint32 width,
    gint32 height);

G_DECLARE_INTERFACE(FlAtspiDBusSocket,
                    fl_atspi_dbus_socket,
                    FL,
                    ATSPI_DBUS_SOCKET,
                    GObject);

/**
 * FlAtspiDBusSocket:
 *
 * #FlAtspiDBusSocket is implemented by objects that handle the
 * org.a11y.atspi.Socket D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_socket_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusSocketInterface {
  GTypeInterface g_iface;

  gboolean (*handle_embedded)(FlAtspiDBusSocket* self,
                              GDBusMethodInvocation* invocation,
                              const gchar* path);
};

/**
 * fl_atspi_dbus_socket_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Socket.
 */
GDBusInterfaceInfo* fl_atspi_dbus_socket_interface_info();

/**
 * fl_atspi_dbus_socket_register:
 * @self: an #FlAtspiDBusSocket.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_socket_register(FlAtspiDBusSocket* self,
                                    GDBusConnection* connection,
                                    const gchar* object_path,
                                    GError** error);

/**
 * fl_atspi_dbus_socket_complete_embedded:
 * @self: an #FlAtspiDBusSocket.
 * @invocation: (transfer full): the invocation being handled.
 *
 * Completes handling the Embedded() method.
 */
void fl_atspi_dbus_socket_complete_embedded(FlAtspiDBusSocket* self,
                                            GDBusMethodInvocation* invocation);

G_DECLARE_INTERFACE(FlAtspiDBusCache,
                    fl_atspi_dbus_cache,
                    FL,
                    ATSPI_DBUS_CACHE,
                    GObject);

/**
 * FlAtspiDBusCache:
 *
 * #FlAtspiDBusCache is implemented by objects that handle the
 * org.a11y.atspi.Cache D-Bus interface. Method
 * handlers must complete the call, e.g. with fl_atspi_dbus_cache_complete_*(),
 * and return %TRUE. Methods and properties without an implementation return an
 * error.
 */

struct _FlAtspiDBusCacheInterface {
  GTypeInterface g_iface;

  gboolean (*handle_get_items)(FlAtspiDBusCache* self,
                               GDBusMethodInvocation* invocation);
};

/**
 * fl_atspi_dbus_cache_interface_info:
 *
 * Returns: (transfer none): the description of org.a11y.atspi.Cache.
 */
GDBusInterfaceInfo* fl_atspi_dbus_cache_interface_info();

/**
 * fl_atspi_dbus_cache_register:
 * @self: an #FlAtspiDBusCache.
 * @connection: a #GDBusConnection.
 * @object_path: the object path to export @self at.
 * @error: (allow-none): #GError location to store the error occurring, or
 * %NULL.
 *
 * Exports @self on @connection until it is unregistered with
 * g_dbus_connection_unregister_object(). @self is not referenced, so it must
 * be unregistered before it is destroyed.
 *
 * Returns: the registration ID or 0 on error.
 */
guint fl_atspi_dbus_cache_register(FlAtspiDBusCache* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error);

/**
 * fl_atspi_dbus_cache_complete_get_items:
 * @self: an #FlAtspiDBusCache.
 * @invocation: (transfer full): the invocation being handled.
 * @nodes: the nodes argument.
 *
 * Completes handling the GetItems() method.
 */
void fl_atspi_dbus_cache_complete_get_items(FlAtspiDBusCache* self,
                                            GDBusMethodInvocation* invocation,
                                            GVariant* nodes);

/**
 * fl_atspi_dbus_event_object_emit_property_change:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the PropertyChange signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_property_change(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_state_changed:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the StateChanged signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_state_changed(GDBusConnection* connection,
                                                   const gchar* object_path,
                                                   const gchar* detail,
                                                   gint32 detail1,
                                                   gint32 detail2,
                                                   GVariant* any_data,
                                                   GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_children_changed:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the ChildrenChanged signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_children_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_selection_changed:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the SelectionChanged signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_selection_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_announcement:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the Announcement signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_announcement(GDBusConnection* connection,
                                                  const gchar* object_path,
                                                  const gchar* detail,
                                                  gint32 detail1,
                                                  gint32 detail2,
                                                  GVariant* any_data,
                                                  GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_text_selection_changed:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the TextSelectionChanged signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_text_selection_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_text_changed:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the TextChanged signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_text_changed(GDBusConnection* connection,
                                                  const gchar* object_path,
                                                  const gchar* detail,
                                                  gint32 detail1,
                                                  gint32 detail2,
                                                  GVariant* any_data,
                                                  GVariant* properties);

/**
 * fl_atspi_dbus_event_object_emit_text_caret_moved:
 * @connection: a #GDBusConnection.
 * @object_path: the object path of the object emitting the signal.
 * @detail: the detail argument.
 * @detail1: the detail1 argument.
 * @detail2: the detail2 argument.
 * @any_data: the any_data argument.
 * @properties: the properties argument.
 *
 * Emits the TextCaretMoved signal on org.a11y.atspi.Event.Object.
 */
void fl_atspi_dbus_event_object_emit_text_caret_moved(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_DBUS_H_
