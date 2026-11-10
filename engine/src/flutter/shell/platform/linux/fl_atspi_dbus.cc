// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_dbus.h"

// The AT-SPI interfaces used, from at-spi2-core's xml directory.
static constexpr char kIntrospectionXml[] = R"xml(<node>
  <interface name="org.a11y.Bus">
    <method name="GetAddress">
      <arg direction="out" name="address" type="s"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Registry">
    <method name="GetRegisteredEvents">
      <arg direction="out" name="events" type="a(ss)"/>
    </method>
    <signal name="EventListenerRegistered">
      <arg name="bus" type="s"/>
      <arg name="event" type="s"/>
      <arg name="properties" type="as"/>
    </signal>
    <signal name="EventListenerDeregistered">
      <arg name="bus" type="s"/>
      <arg name="event" type="s"/>
    </signal>
  </interface>
  <interface name="org.a11y.atspi.Accessible">
    <property name="Name" type="s" access="read"/>
    <property name="Description" type="s" access="read"/>
    <property name="Parent" type="(so)" access="read"/>
    <property name="ChildCount" type="i" access="read"/>
    <property name="Locale" type="s" access="read"/>
    <property name="AccessibleId" type="s" access="read"/>
    <property name="HelpText" type="s" access="read"/>
    <method name="GetChildAtIndex">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="child" type="(so)"/>
    </method>
    <method name="GetChildren">
      <arg direction="out" name="children" type="a(so)"/>
    </method>
    <method name="GetIndexInParent">
      <arg direction="out" name="index" type="i"/>
    </method>
    <method name="GetRelationSet">
      <arg direction="out" name="relations" type="a(ua(so))"/>
    </method>
    <method name="GetRole">
      <arg direction="out" name="role" type="u"/>
    </method>
    <method name="GetRoleName">
      <arg direction="out" name="name" type="s"/>
    </method>
    <method name="GetLocalizedRoleName">
      <arg direction="out" name="name" type="s"/>
    </method>
    <method name="GetState">
      <arg direction="out" name="state" type="au"/>
    </method>
    <method name="GetAttributes">
      <arg direction="out" name="attributes" type="a{ss}"/>
    </method>
    <method name="GetApplication">
      <arg direction="out" name="application" type="(so)"/>
    </method>
    <method name="GetInterfaces">
      <arg direction="out" name="interfaces" type="as"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Application">
    <property name="ToolkitName" type="s" access="read"/>
    <property name="Version" type="s" access="read"/>
    <property name="ToolkitVersion" type="s" access="read"/>
    <property name="AtspiVersion" type="s" access="read"/>
    <property name="InterfaceVersion" type="u" access="read"/>
    <property name="Id" type="i" access="readwrite"/>
    <method name="GetLocale">
      <arg direction="in" name="lctype" type="u"/>
      <arg direction="out" name="locale" type="s"/>
    </method>
    <method name="GetApplicationBusAddress">
      <arg direction="out" name="address" type="s"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Component">
    <method name="Contains">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="contains" type="b"/>
    </method>
    <method name="GetAccessibleAtPoint">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="accessible" type="(so)"/>
    </method>
    <method name="GetExtents">
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="extents" type="(iiii)"/>
    </method>
    <method name="GetPosition">
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="x" type="i"/>
      <arg direction="out" name="y" type="i"/>
    </method>
    <method name="GetSize">
      <arg direction="out" name="width" type="i"/>
      <arg direction="out" name="height" type="i"/>
    </method>
    <method name="GetLayer">
      <arg direction="out" name="layer" type="u"/>
    </method>
    <method name="GetMDIZOrder">
      <arg direction="out" name="z_order" type="n"/>
    </method>
    <method name="GrabFocus">
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="GetAlpha">
      <arg direction="out" name="alpha" type="d"/>
    </method>
    <method name="SetExtents">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="width" type="i"/>
      <arg direction="in" name="height" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="SetPosition">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="SetSize">
      <arg direction="in" name="width" type="i"/>
      <arg direction="in" name="height" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="ScrollTo">
      <arg direction="in" name="type" type="u"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="ScrollToPoint">
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Action">
    <property name="NActions" type="i" access="read"/>
    <method name="GetDescription">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="description" type="s"/>
    </method>
    <method name="GetName">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="name" type="s"/>
    </method>
    <method name="GetLocalizedName">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="name" type="s"/>
    </method>
    <method name="GetKeyBinding">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="key_binding" type="s"/>
    </method>
    <method name="GetActions">
      <arg direction="out" name="actions" type="a(sss)"/>
    </method>
    <method name="DoAction">
      <arg direction="in" name="index" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Value">
    <property name="MinimumValue" type="d" access="read"/>
    <property name="MaximumValue" type="d" access="read"/>
    <property name="MinimumIncrement" type="d" access="read"/>
    <property name="CurrentValue" type="d" access="readwrite"/>
    <property name="Text" type="s" access="read"/>
  </interface>
  <interface name="org.a11y.atspi.Text">
    <property name="CharacterCount" type="i" access="read"/>
    <property name="CaretOffset" type="i" access="read"/>
    <method name="GetStringAtOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="granularity" type="u"/>
      <arg direction="out" name="text" type="s"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetText">
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="out" name="text" type="s"/>
    </method>
    <method name="SetCaretOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="GetTextBeforeOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="type" type="u"/>
      <arg direction="out" name="text" type="s"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetTextAtOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="type" type="u"/>
      <arg direction="out" name="text" type="s"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetTextAfterOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="type" type="u"/>
      <arg direction="out" name="text" type="s"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetCharacterAtOffset">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="out" name="character" type="i"/>
    </method>
    <method name="GetAttributeValue">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="attribute_name" type="s"/>
      <arg direction="out" name="value" type="s"/>
    </method>
    <method name="GetAttributes">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="out" name="attributes" type="a{ss}"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetDefaultAttributes">
      <arg direction="out" name="attributes" type="a{ss}"/>
    </method>
    <method name="GetCharacterExtents">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="x" type="i"/>
      <arg direction="out" name="y" type="i"/>
      <arg direction="out" name="width" type="i"/>
      <arg direction="out" name="height" type="i"/>
    </method>
    <method name="GetOffsetAtPoint">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="offset" type="i"/>
    </method>
    <method name="GetNSelections">
      <arg direction="out" name="n_selections" type="i"/>
    </method>
    <method name="GetSelection">
      <arg direction="in" name="selection_num" type="i"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="AddSelection">
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="RemoveSelection">
      <arg direction="in" name="selection_num" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="SetSelection">
      <arg direction="in" name="selection_num" type="i"/>
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="GetRangeExtents">
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="x" type="i"/>
      <arg direction="out" name="y" type="i"/>
      <arg direction="out" name="width" type="i"/>
      <arg direction="out" name="height" type="i"/>
    </method>
    <method name="GetBoundedRanges">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="in" name="width" type="i"/>
      <arg direction="in" name="height" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="in" name="x_clip_type" type="u"/>
      <arg direction="in" name="y_clip_type" type="u"/>
      <arg direction="out" name="ranges" type="a(iisv)"/>
    </method>
    <method name="GetAttributeRun">
      <arg direction="in" name="offset" type="i"/>
      <arg direction="in" name="include_defaults" type="b"/>
      <arg direction="out" name="attributes" type="a{ss}"/>
      <arg direction="out" name="start_offset" type="i"/>
      <arg direction="out" name="end_offset" type="i"/>
    </method>
    <method name="GetDefaultAttributeSet">
      <arg direction="out" name="attributes" type="a{ss}"/>
    </method>
    <method name="ScrollSubstringTo">
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="in" name="type" type="u"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="ScrollSubstringToPoint">
      <arg direction="in" name="start_offset" type="i"/>
      <arg direction="in" name="end_offset" type="i"/>
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.EditableText">
    <method name="SetTextContents">
      <arg direction="in" name="new_contents" type="s"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="InsertText">
      <arg direction="in" name="position" type="i"/>
      <arg direction="in" name="text" type="s"/>
      <arg direction="in" name="length" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="CopyText">
      <arg direction="in" name="start_pos" type="i"/>
      <arg direction="in" name="end_pos" type="i"/>
    </method>
    <method name="CutText">
      <arg direction="in" name="start_pos" type="i"/>
      <arg direction="in" name="end_pos" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="DeleteText">
      <arg direction="in" name="start_pos" type="i"/>
      <arg direction="in" name="end_pos" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="PasteText">
      <arg direction="in" name="position" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Selection">
    <property name="NSelectedChildren" type="i" access="read"/>
    <method name="GetSelectedChild">
      <arg direction="in" name="selected_child_index" type="i"/>
      <arg direction="out" name="child" type="(so)"/>
    </method>
    <method name="SelectChild">
      <arg direction="in" name="child_index" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="DeselectSelectedChild">
      <arg direction="in" name="selected_child_index" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="IsChildSelected">
      <arg direction="in" name="child_index" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="SelectAll">
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="ClearSelection">
      <arg direction="out" name="result" type="b"/>
    </method>
    <method name="DeselectChild">
      <arg direction="in" name="child_index" type="i"/>
      <arg direction="out" name="result" type="b"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Image">
    <property name="ImageDescription" type="s" access="read"/>
    <property name="ImageLocale" type="s" access="read"/>
    <method name="GetImageExtents">
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="extents" type="(iiii)"/>
    </method>
    <method name="GetImagePosition">
      <arg direction="in" name="coord_type" type="u"/>
      <arg direction="out" name="x" type="i"/>
      <arg direction="out" name="y" type="i"/>
    </method>
    <method name="GetImageSize">
      <arg direction="out" name="width" type="i"/>
      <arg direction="out" name="height" type="i"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Socket">
    <method name="Embedded">
      <arg direction="in" name="path" type="s"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Cache">
    <method name="GetItems">
      <arg direction="out" name="nodes" type="a((so)(so)(so)iiassusau)"/>
    </method>
  </interface>
  <interface name="org.a11y.atspi.Event.Object">
    <signal name="PropertyChange">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="StateChanged">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="ChildrenChanged">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="SelectionChanged">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="Announcement">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="TextSelectionChanged">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="TextChanged">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
    <signal name="TextCaretMoved">
      <arg name="detail" type="s"/>
      <arg name="detail1" type="i"/>
      <arg name="detail2" type="i"/>
      <arg name="any_data" type="v"/>
      <arg name="properties" type="a{sv}"/>
    </signal>
  </interface>
</node>)xml";

// A function to call when a signal is received.
typedef struct {
  GCallback callback;
  gpointer user_data;
} SignalHandler;

static SignalHandler* signal_handler_new(GCallback callback,
                                         gpointer user_data) {
  SignalHandler* handler = g_new0(SignalHandler, 1);
  handler->callback = callback;
  handler->user_data = user_data;
  return handler;
}

static GDBusInterfaceInfo* lookup_interface_info(const gchar* name) {
  static GDBusNodeInfo* info = nullptr;
  if (info == nullptr) {
    g_autoptr(GError) error = nullptr;
    info = g_dbus_node_info_new_for_xml(kIntrospectionXml, &error);
    if (info == nullptr) {
      g_error("Failed to parse AT-SPI introspection data: %s", error->message);
    }
  }
  return g_dbus_node_info_lookup_interface(info, name);
}

// Returns a string variant for |value|, taking ownership of it.
static GVariant* take_string(gchar* value) {
  return g_variant_new_take_string(value != nullptr ? value : g_strdup(""));
}

void fl_atspi_dbus_bus_call_get_address(GDBusConnection* connection,
                                        const gchar* bus_name,
                                        const gchar* object_path,
                                        GCancellable* cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data) {
  g_dbus_connection_call(connection, bus_name, object_path, "org.a11y.Bus",
                         "GetAddress", nullptr, G_VARIANT_TYPE("(s)"),
                         G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback,
                         user_data);
}

gboolean fl_atspi_dbus_bus_call_get_address_finish(GDBusConnection* connection,
                                                   GAsyncResult* result,
                                                   gchar** address,
                                                   GError** error) {
  g_autoptr(GVariant) reply =
      g_dbus_connection_call_finish(connection, result, error);
  if (reply == nullptr) {
    return FALSE;
  }
  g_variant_get(reply, "(s)", address);
  return TRUE;
}

void fl_atspi_dbus_registry_call_get_registered_events(
    GDBusConnection* connection,
    const gchar* bus_name,
    const gchar* object_path,
    GCancellable* cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data) {
  g_dbus_connection_call(
      connection, bus_name, object_path, "org.a11y.atspi.Registry",
      "GetRegisteredEvents", nullptr, G_VARIANT_TYPE("(a(ss))"),
      G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
}

gboolean fl_atspi_dbus_registry_call_get_registered_events_finish(
    GDBusConnection* connection,
    GAsyncResult* result,
    GVariant** events,
    GError** error) {
  g_autoptr(GVariant) reply =
      g_dbus_connection_call_finish(connection, result, error);
  if (reply == nullptr) {
    return FALSE;
  }
  g_variant_get(reply, "(@a(ss))", events);
  return TRUE;
}

static void fl_atspi_dbus_registry_subscribe_event_listener_registered_cb(
    GDBusConnection* connection,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* parameters,
    gpointer user_data) {
  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ssas)"))) {
    return;
  }
  SignalHandler* handler = static_cast<SignalHandler*>(user_data);
  const gchar* bus;
  const gchar* event;
  g_autofree const gchar** properties = nullptr;
  g_variant_get(parameters, "(&s&s^a&s)", &bus, &event, &properties);
  reinterpret_cast<FlAtspiDBusRegistryEventListenerRegisteredHandler>(
      handler->callback)(bus, event,
                         static_cast<const gchar* const*>(properties),
                         handler->user_data);
}

guint fl_atspi_dbus_registry_subscribe_event_listener_registered(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    FlAtspiDBusRegistryEventListenerRegisteredHandler handler,
    gpointer user_data) {
  return g_dbus_connection_signal_subscribe(
      connection, sender, "org.a11y.atspi.Registry", "EventListenerRegistered",
      object_path, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      fl_atspi_dbus_registry_subscribe_event_listener_registered_cb,
      signal_handler_new(reinterpret_cast<GCallback>(handler), user_data),
      g_free);
}

static void fl_atspi_dbus_registry_subscribe_event_listener_deregistered_cb(
    GDBusConnection* connection,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* parameters,
    gpointer user_data) {
  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ss)"))) {
    return;
  }
  SignalHandler* handler = static_cast<SignalHandler*>(user_data);
  const gchar* bus;
  const gchar* event;
  g_variant_get(parameters, "(&s&s)", &bus, &event);
  reinterpret_cast<FlAtspiDBusRegistryEventListenerDeregisteredHandler>(
      handler->callback)(bus, event, handler->user_data);
}

guint fl_atspi_dbus_registry_subscribe_event_listener_deregistered(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    FlAtspiDBusRegistryEventListenerDeregisteredHandler handler,
    gpointer user_data) {
  return g_dbus_connection_signal_subscribe(
      connection, sender, "org.a11y.atspi.Registry",
      "EventListenerDeregistered", object_path, nullptr,
      G_DBUS_SIGNAL_FLAGS_NONE,
      fl_atspi_dbus_registry_subscribe_event_listener_deregistered_cb,
      signal_handler_new(reinterpret_cast<GCallback>(handler), user_data),
      g_free);
}

G_DEFINE_INTERFACE(FlAtspiDBusAccessible,
                   fl_atspi_dbus_accessible,
                   G_TYPE_OBJECT)

static void fl_atspi_dbus_accessible_default_init(
    FlAtspiDBusAccessibleInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_accessible_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Accessible");
}

static void fl_atspi_dbus_accessible_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
  FlAtspiDBusAccessible* self = FL_ATSPI_DBUS_ACCESSIBLE(user_data);
  FlAtspiDBusAccessibleInterface* iface =
      FL_ATSPI_DBUS_ACCESSIBLE_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetChildAtIndex") == 0) {
    if (iface->handle_get_child_at_index != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_get_child_at_index(self, invocation, index);
    }
  } else if (g_strcmp0(method_name, "GetChildren") == 0) {
    if (iface->handle_get_children != nullptr) {
      handled = iface->handle_get_children(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetIndexInParent") == 0) {
    if (iface->handle_get_index_in_parent != nullptr) {
      handled = iface->handle_get_index_in_parent(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetRelationSet") == 0) {
    if (iface->handle_get_relation_set != nullptr) {
      handled = iface->handle_get_relation_set(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetRole") == 0) {
    if (iface->handle_get_role != nullptr) {
      handled = iface->handle_get_role(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetRoleName") == 0) {
    if (iface->handle_get_role_name != nullptr) {
      handled = iface->handle_get_role_name(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetLocalizedRoleName") == 0) {
    if (iface->handle_get_localized_role_name != nullptr) {
      handled = iface->handle_get_localized_role_name(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetState") == 0) {
    if (iface->handle_get_state != nullptr) {
      handled = iface->handle_get_state(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetAttributes") == 0) {
    if (iface->handle_get_attributes != nullptr) {
      handled = iface->handle_get_attributes(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetApplication") == 0) {
    if (iface->handle_get_application != nullptr) {
      handled = iface->handle_get_application(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetInterfaces") == 0) {
    if (iface->handle_get_interfaces != nullptr) {
      handled = iface->handle_get_interfaces(self, invocation);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_accessible_get_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GError** error,
    gpointer user_data) {
  FlAtspiDBusAccessible* self = FL_ATSPI_DBUS_ACCESSIBLE(user_data);
  FlAtspiDBusAccessibleInterface* iface =
      FL_ATSPI_DBUS_ACCESSIBLE_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "Name") == 0 && iface->get_name != nullptr) {
    return take_string(iface->get_name(self));
  } else if (g_strcmp0(property_name, "Description") == 0 &&
             iface->get_description != nullptr) {
    return take_string(iface->get_description(self));
  } else if (g_strcmp0(property_name, "Parent") == 0 &&
             iface->get_parent != nullptr) {
    value = iface->get_parent(self);
  } else if (g_strcmp0(property_name, "ChildCount") == 0 &&
             iface->get_child_count != nullptr) {
    return g_variant_new("i", iface->get_child_count(self));
  } else if (g_strcmp0(property_name, "Locale") == 0 &&
             iface->get_locale != nullptr) {
    return take_string(iface->get_locale(self));
  } else if (g_strcmp0(property_name, "AccessibleId") == 0 &&
             iface->get_accessible_id != nullptr) {
    return take_string(iface->get_accessible_id(self));
  } else if (g_strcmp0(property_name, "HelpText") == 0 &&
             iface->get_help_text != nullptr) {
    return take_string(iface->get_help_text(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_accessible_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_accessible_method_call,
      .get_property = fl_atspi_dbus_accessible_get_property,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_accessible_register(FlAtspiDBusAccessible* self,
                                        GDBusConnection* connection,
                                        const gchar* object_path,
                                        GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_accessible_interface_info(),
      fl_atspi_dbus_accessible_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_accessible_complete_get_child_at_index(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* child) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(so))", child));
}

void fl_atspi_dbus_accessible_complete_get_children(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* children) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a(so))", children));
}

void fl_atspi_dbus_accessible_complete_get_index_in_parent(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    gint32 index) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(i)", index));
}

void fl_atspi_dbus_accessible_complete_get_relation_set(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* relations) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(@a(ua(so)))", relations));
}

void fl_atspi_dbus_accessible_complete_get_role(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    guint32 role) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", role));
}

void fl_atspi_dbus_accessible_complete_get_role_name(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* name) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", name));
}

void fl_atspi_dbus_accessible_complete_get_localized_role_name(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* name) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", name));
}

void fl_atspi_dbus_accessible_complete_get_state(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* state) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@au)", state));
}

void fl_atspi_dbus_accessible_complete_get_attributes(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a{ss})", attributes));
}

void fl_atspi_dbus_accessible_complete_get_application(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    GVariant* application) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(so))", application));
}

void fl_atspi_dbus_accessible_complete_get_interfaces(
    FlAtspiDBusAccessible* self,
    GDBusMethodInvocation* invocation,
    const gchar* const* interfaces) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(^as)", interfaces));
}

G_DEFINE_INTERFACE(FlAtspiDBusApplication,
                   fl_atspi_dbus_application,
                   G_TYPE_OBJECT)

static void fl_atspi_dbus_application_default_init(
    FlAtspiDBusApplicationInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_application_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Application");
}

static void fl_atspi_dbus_application_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
  FlAtspiDBusApplication* self = FL_ATSPI_DBUS_APPLICATION(user_data);
  FlAtspiDBusApplicationInterface* iface =
      FL_ATSPI_DBUS_APPLICATION_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetLocale") == 0) {
    if (iface->handle_get_locale != nullptr) {
      guint32 lctype;
      g_variant_get(parameters, "(u)", &lctype);
      handled = iface->handle_get_locale(self, invocation, lctype);
    }
  } else if (g_strcmp0(method_name, "GetApplicationBusAddress") == 0) {
    if (iface->handle_get_application_bus_address != nullptr) {
      handled = iface->handle_get_application_bus_address(self, invocation);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_application_get_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GError** error,
    gpointer user_data) {
  FlAtspiDBusApplication* self = FL_ATSPI_DBUS_APPLICATION(user_data);
  FlAtspiDBusApplicationInterface* iface =
      FL_ATSPI_DBUS_APPLICATION_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "ToolkitName") == 0 &&
      iface->get_toolkit_name != nullptr) {
    return take_string(iface->get_toolkit_name(self));
  } else if (g_strcmp0(property_name, "Version") == 0 &&
             iface->get_version != nullptr) {
    return take_string(iface->get_version(self));
  } else if (g_strcmp0(property_name, "ToolkitVersion") == 0 &&
             iface->get_toolkit_version != nullptr) {
    return take_string(iface->get_toolkit_version(self));
  } else if (g_strcmp0(property_name, "AtspiVersion") == 0 &&
             iface->get_atspi_version != nullptr) {
    return take_string(iface->get_atspi_version(self));
  } else if (g_strcmp0(property_name, "InterfaceVersion") == 0 &&
             iface->get_interface_version != nullptr) {
    return g_variant_new("u", iface->get_interface_version(self));
  } else if (g_strcmp0(property_name, "Id") == 0 && iface->get_id != nullptr) {
    return g_variant_new("i", iface->get_id(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static gboolean fl_atspi_dbus_application_set_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GVariant* value,
    GError** error,
    gpointer user_data) {
  FlAtspiDBusApplication* self = FL_ATSPI_DBUS_APPLICATION(user_data);
  FlAtspiDBusApplicationInterface* iface =
      FL_ATSPI_DBUS_APPLICATION_GET_IFACE(self);
  if (g_strcmp0(property_name, "Id") == 0 && iface->set_id != nullptr) {
    gint32 v;
    g_variant_get(value, "i", &v);
    iface->set_id(self, v);
    return TRUE;
  }
  g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_PROPERTY_READ_ONLY,
              "Property %s.%s is not writable", interface_name, property_name);
  return FALSE;
}

static const GDBusInterfaceVTable*
fl_atspi_dbus_application_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_application_method_call,
      .get_property = fl_atspi_dbus_application_get_property,
      .set_property = fl_atspi_dbus_application_set_property,
  };
  return &vtable;
}

guint fl_atspi_dbus_application_register(FlAtspiDBusApplication* self,
                                         GDBusConnection* connection,
                                         const gchar* object_path,
                                         GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_application_interface_info(),
      fl_atspi_dbus_application_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_application_complete_get_locale(
    FlAtspiDBusApplication* self,
    GDBusMethodInvocation* invocation,
    const gchar* locale) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", locale));
}

void fl_atspi_dbus_application_complete_get_application_bus_address(
    FlAtspiDBusApplication* self,
    GDBusMethodInvocation* invocation,
    const gchar* address) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", address));
}

G_DEFINE_INTERFACE(FlAtspiDBusComponent, fl_atspi_dbus_component, G_TYPE_OBJECT)

static void fl_atspi_dbus_component_default_init(
    FlAtspiDBusComponentInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_component_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Component");
}

static void fl_atspi_dbus_component_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
  FlAtspiDBusComponent* self = FL_ATSPI_DBUS_COMPONENT(user_data);
  FlAtspiDBusComponentInterface* iface =
      FL_ATSPI_DBUS_COMPONENT_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "Contains") == 0) {
    if (iface->handle_contains != nullptr) {
      gint32 x;
      gint32 y;
      guint32 coord_type;
      g_variant_get(parameters, "(iiu)", &x, &y, &coord_type);
      handled = iface->handle_contains(self, invocation, x, y, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetAccessibleAtPoint") == 0) {
    if (iface->handle_get_accessible_at_point != nullptr) {
      gint32 x;
      gint32 y;
      guint32 coord_type;
      g_variant_get(parameters, "(iiu)", &x, &y, &coord_type);
      handled = iface->handle_get_accessible_at_point(self, invocation, x, y,
                                                      coord_type);
    }
  } else if (g_strcmp0(method_name, "GetExtents") == 0) {
    if (iface->handle_get_extents != nullptr) {
      guint32 coord_type;
      g_variant_get(parameters, "(u)", &coord_type);
      handled = iface->handle_get_extents(self, invocation, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetPosition") == 0) {
    if (iface->handle_get_position != nullptr) {
      guint32 coord_type;
      g_variant_get(parameters, "(u)", &coord_type);
      handled = iface->handle_get_position(self, invocation, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetSize") == 0) {
    if (iface->handle_get_size != nullptr) {
      handled = iface->handle_get_size(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetLayer") == 0) {
    if (iface->handle_get_layer != nullptr) {
      handled = iface->handle_get_layer(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetMDIZOrder") == 0) {
    if (iface->handle_get_mdizorder != nullptr) {
      handled = iface->handle_get_mdizorder(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GrabFocus") == 0) {
    if (iface->handle_grab_focus != nullptr) {
      handled = iface->handle_grab_focus(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetAlpha") == 0) {
    if (iface->handle_get_alpha != nullptr) {
      handled = iface->handle_get_alpha(self, invocation);
    }
  } else if (g_strcmp0(method_name, "SetExtents") == 0) {
    if (iface->handle_set_extents != nullptr) {
      gint32 x;
      gint32 y;
      gint32 width;
      gint32 height;
      guint32 coord_type;
      g_variant_get(parameters, "(iiiiu)", &x, &y, &width, &height,
                    &coord_type);
      handled = iface->handle_set_extents(self, invocation, x, y, width, height,
                                          coord_type);
    }
  } else if (g_strcmp0(method_name, "SetPosition") == 0) {
    if (iface->handle_set_position != nullptr) {
      gint32 x;
      gint32 y;
      guint32 coord_type;
      g_variant_get(parameters, "(iiu)", &x, &y, &coord_type);
      handled = iface->handle_set_position(self, invocation, x, y, coord_type);
    }
  } else if (g_strcmp0(method_name, "SetSize") == 0) {
    if (iface->handle_set_size != nullptr) {
      gint32 width;
      gint32 height;
      g_variant_get(parameters, "(ii)", &width, &height);
      handled = iface->handle_set_size(self, invocation, width, height);
    }
  } else if (g_strcmp0(method_name, "ScrollTo") == 0) {
    if (iface->handle_scroll_to != nullptr) {
      guint32 type;
      g_variant_get(parameters, "(u)", &type);
      handled = iface->handle_scroll_to(self, invocation, type);
    }
  } else if (g_strcmp0(method_name, "ScrollToPoint") == 0) {
    if (iface->handle_scroll_to_point != nullptr) {
      guint32 coord_type;
      gint32 x;
      gint32 y;
      g_variant_get(parameters, "(uii)", &coord_type, &x, &y);
      handled =
          iface->handle_scroll_to_point(self, invocation, coord_type, x, y);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static const GDBusInterfaceVTable* fl_atspi_dbus_component_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_component_method_call,
      .get_property = nullptr,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_component_register(FlAtspiDBusComponent* self,
                                       GDBusConnection* connection,
                                       const gchar* object_path,
                                       GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_component_interface_info(),
      fl_atspi_dbus_component_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_component_complete_contains(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean contains) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", contains));
}

void fl_atspi_dbus_component_complete_get_accessible_at_point(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    GVariant* accessible) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(so))", accessible));
}

void fl_atspi_dbus_component_complete_get_extents(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    GVariant* extents) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(iiii))", extents));
}

void fl_atspi_dbus_component_complete_get_position(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(ii)", x, y));
}

void fl_atspi_dbus_component_complete_get_size(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint32 width,
    gint32 height) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(ii)", width, height));
}

void fl_atspi_dbus_component_complete_get_layer(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    guint32 layer) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(u)", layer));
}

void fl_atspi_dbus_component_complete_get_mdizorder(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gint16 z_order) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(n)", z_order));
}

void fl_atspi_dbus_component_complete_grab_focus(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_component_complete_get_alpha(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gdouble alpha) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(d)", alpha));
}

void fl_atspi_dbus_component_complete_set_extents(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_component_complete_set_position(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_component_complete_set_size(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_component_complete_scroll_to(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_component_complete_scroll_to_point(
    FlAtspiDBusComponent* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

G_DEFINE_INTERFACE(FlAtspiDBusAction, fl_atspi_dbus_action, G_TYPE_OBJECT)

static void fl_atspi_dbus_action_default_init(
    FlAtspiDBusActionInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_action_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Action");
}

static void fl_atspi_dbus_action_method_call(GDBusConnection* connection,
                                             const gchar* sender,
                                             const gchar* object_path,
                                             const gchar* interface_name,
                                             const gchar* method_name,
                                             GVariant* parameters,
                                             GDBusMethodInvocation* invocation,
                                             gpointer user_data) {
  FlAtspiDBusAction* self = FL_ATSPI_DBUS_ACTION(user_data);
  FlAtspiDBusActionInterface* iface = FL_ATSPI_DBUS_ACTION_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetDescription") == 0) {
    if (iface->handle_get_description != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_get_description(self, invocation, index);
    }
  } else if (g_strcmp0(method_name, "GetName") == 0) {
    if (iface->handle_get_name != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_get_name(self, invocation, index);
    }
  } else if (g_strcmp0(method_name, "GetLocalizedName") == 0) {
    if (iface->handle_get_localized_name != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_get_localized_name(self, invocation, index);
    }
  } else if (g_strcmp0(method_name, "GetKeyBinding") == 0) {
    if (iface->handle_get_key_binding != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_get_key_binding(self, invocation, index);
    }
  } else if (g_strcmp0(method_name, "GetActions") == 0) {
    if (iface->handle_get_actions != nullptr) {
      handled = iface->handle_get_actions(self, invocation);
    }
  } else if (g_strcmp0(method_name, "DoAction") == 0) {
    if (iface->handle_do_action != nullptr) {
      gint32 index;
      g_variant_get(parameters, "(i)", &index);
      handled = iface->handle_do_action(self, invocation, index);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_action_get_property(GDBusConnection* connection,
                                                   const gchar* sender,
                                                   const gchar* object_path,
                                                   const gchar* interface_name,
                                                   const gchar* property_name,
                                                   GError** error,
                                                   gpointer user_data) {
  FlAtspiDBusAction* self = FL_ATSPI_DBUS_ACTION(user_data);
  FlAtspiDBusActionInterface* iface = FL_ATSPI_DBUS_ACTION_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "NActions") == 0 &&
      iface->get_nactions != nullptr) {
    return g_variant_new("i", iface->get_nactions(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_action_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_action_method_call,
      .get_property = fl_atspi_dbus_action_get_property,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_action_register(FlAtspiDBusAction* self,
                                    GDBusConnection* connection,
                                    const gchar* object_path,
                                    GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_action_interface_info(),
      fl_atspi_dbus_action_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_action_complete_get_description(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* description) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", description));
}

void fl_atspi_dbus_action_complete_get_name(FlAtspiDBusAction* self,
                                            GDBusMethodInvocation* invocation,
                                            const gchar* name) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", name));
}

void fl_atspi_dbus_action_complete_get_localized_name(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* name) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", name));
}

void fl_atspi_dbus_action_complete_get_key_binding(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    const gchar* key_binding) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", key_binding));
}

void fl_atspi_dbus_action_complete_get_actions(
    FlAtspiDBusAction* self,
    GDBusMethodInvocation* invocation,
    GVariant* actions) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a(sss))", actions));
}

void fl_atspi_dbus_action_complete_do_action(FlAtspiDBusAction* self,
                                             GDBusMethodInvocation* invocation,
                                             gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

G_DEFINE_INTERFACE(FlAtspiDBusValue, fl_atspi_dbus_value, G_TYPE_OBJECT)

static void fl_atspi_dbus_value_default_init(FlAtspiDBusValueInterface* iface) {
}

GDBusInterfaceInfo* fl_atspi_dbus_value_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Value");
}

static GVariant* fl_atspi_dbus_value_get_property(GDBusConnection* connection,
                                                  const gchar* sender,
                                                  const gchar* object_path,
                                                  const gchar* interface_name,
                                                  const gchar* property_name,
                                                  GError** error,
                                                  gpointer user_data) {
  FlAtspiDBusValue* self = FL_ATSPI_DBUS_VALUE(user_data);
  FlAtspiDBusValueInterface* iface = FL_ATSPI_DBUS_VALUE_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "MinimumValue") == 0 &&
      iface->get_minimum_value != nullptr) {
    return g_variant_new("d", iface->get_minimum_value(self));
  } else if (g_strcmp0(property_name, "MaximumValue") == 0 &&
             iface->get_maximum_value != nullptr) {
    return g_variant_new("d", iface->get_maximum_value(self));
  } else if (g_strcmp0(property_name, "MinimumIncrement") == 0 &&
             iface->get_minimum_increment != nullptr) {
    return g_variant_new("d", iface->get_minimum_increment(self));
  } else if (g_strcmp0(property_name, "CurrentValue") == 0 &&
             iface->get_current_value != nullptr) {
    return g_variant_new("d", iface->get_current_value(self));
  } else if (g_strcmp0(property_name, "Text") == 0 &&
             iface->get_text != nullptr) {
    return take_string(iface->get_text(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static gboolean fl_atspi_dbus_value_set_property(GDBusConnection* connection,
                                                 const gchar* sender,
                                                 const gchar* object_path,
                                                 const gchar* interface_name,
                                                 const gchar* property_name,
                                                 GVariant* value,
                                                 GError** error,
                                                 gpointer user_data) {
  FlAtspiDBusValue* self = FL_ATSPI_DBUS_VALUE(user_data);
  FlAtspiDBusValueInterface* iface = FL_ATSPI_DBUS_VALUE_GET_IFACE(self);
  if (g_strcmp0(property_name, "CurrentValue") == 0 &&
      iface->set_current_value != nullptr) {
    gdouble v;
    g_variant_get(value, "d", &v);
    iface->set_current_value(self, v);
    return TRUE;
  }
  g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_PROPERTY_READ_ONLY,
              "Property %s.%s is not writable", interface_name, property_name);
  return FALSE;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_value_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = nullptr,
      .get_property = fl_atspi_dbus_value_get_property,
      .set_property = fl_atspi_dbus_value_set_property,
  };
  return &vtable;
}

guint fl_atspi_dbus_value_register(FlAtspiDBusValue* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_value_interface_info(),
      fl_atspi_dbus_value_interface_vtable(), self, nullptr, error);
}

G_DEFINE_INTERFACE(FlAtspiDBusText, fl_atspi_dbus_text, G_TYPE_OBJECT)

static void fl_atspi_dbus_text_default_init(FlAtspiDBusTextInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_text_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Text");
}

static void fl_atspi_dbus_text_method_call(GDBusConnection* connection,
                                           const gchar* sender,
                                           const gchar* object_path,
                                           const gchar* interface_name,
                                           const gchar* method_name,
                                           GVariant* parameters,
                                           GDBusMethodInvocation* invocation,
                                           gpointer user_data) {
  FlAtspiDBusText* self = FL_ATSPI_DBUS_TEXT(user_data);
  FlAtspiDBusTextInterface* iface = FL_ATSPI_DBUS_TEXT_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetStringAtOffset") == 0) {
    if (iface->handle_get_string_at_offset != nullptr) {
      gint32 offset;
      guint32 granularity;
      g_variant_get(parameters, "(iu)", &offset, &granularity);
      handled = iface->handle_get_string_at_offset(self, invocation, offset,
                                                   granularity);
    }
  } else if (g_strcmp0(method_name, "GetText") == 0) {
    if (iface->handle_get_text != nullptr) {
      gint32 start_offset;
      gint32 end_offset;
      g_variant_get(parameters, "(ii)", &start_offset, &end_offset);
      handled =
          iface->handle_get_text(self, invocation, start_offset, end_offset);
    }
  } else if (g_strcmp0(method_name, "SetCaretOffset") == 0) {
    if (iface->handle_set_caret_offset != nullptr) {
      gint32 offset;
      g_variant_get(parameters, "(i)", &offset);
      handled = iface->handle_set_caret_offset(self, invocation, offset);
    }
  } else if (g_strcmp0(method_name, "GetTextBeforeOffset") == 0) {
    if (iface->handle_get_text_before_offset != nullptr) {
      gint32 offset;
      guint32 type;
      g_variant_get(parameters, "(iu)", &offset, &type);
      handled =
          iface->handle_get_text_before_offset(self, invocation, offset, type);
    }
  } else if (g_strcmp0(method_name, "GetTextAtOffset") == 0) {
    if (iface->handle_get_text_at_offset != nullptr) {
      gint32 offset;
      guint32 type;
      g_variant_get(parameters, "(iu)", &offset, &type);
      handled =
          iface->handle_get_text_at_offset(self, invocation, offset, type);
    }
  } else if (g_strcmp0(method_name, "GetTextAfterOffset") == 0) {
    if (iface->handle_get_text_after_offset != nullptr) {
      gint32 offset;
      guint32 type;
      g_variant_get(parameters, "(iu)", &offset, &type);
      handled =
          iface->handle_get_text_after_offset(self, invocation, offset, type);
    }
  } else if (g_strcmp0(method_name, "GetCharacterAtOffset") == 0) {
    if (iface->handle_get_character_at_offset != nullptr) {
      gint32 offset;
      g_variant_get(parameters, "(i)", &offset);
      handled = iface->handle_get_character_at_offset(self, invocation, offset);
    }
  } else if (g_strcmp0(method_name, "GetAttributeValue") == 0) {
    if (iface->handle_get_attribute_value != nullptr) {
      gint32 offset;
      const gchar* attribute_name;
      g_variant_get(parameters, "(i&s)", &offset, &attribute_name);
      handled = iface->handle_get_attribute_value(self, invocation, offset,
                                                  attribute_name);
    }
  } else if (g_strcmp0(method_name, "GetAttributes") == 0) {
    if (iface->handle_get_attributes != nullptr) {
      gint32 offset;
      g_variant_get(parameters, "(i)", &offset);
      handled = iface->handle_get_attributes(self, invocation, offset);
    }
  } else if (g_strcmp0(method_name, "GetDefaultAttributes") == 0) {
    if (iface->handle_get_default_attributes != nullptr) {
      handled = iface->handle_get_default_attributes(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetCharacterExtents") == 0) {
    if (iface->handle_get_character_extents != nullptr) {
      gint32 offset;
      guint32 coord_type;
      g_variant_get(parameters, "(iu)", &offset, &coord_type);
      handled = iface->handle_get_character_extents(self, invocation, offset,
                                                    coord_type);
    }
  } else if (g_strcmp0(method_name, "GetOffsetAtPoint") == 0) {
    if (iface->handle_get_offset_at_point != nullptr) {
      gint32 x;
      gint32 y;
      guint32 coord_type;
      g_variant_get(parameters, "(iiu)", &x, &y, &coord_type);
      handled =
          iface->handle_get_offset_at_point(self, invocation, x, y, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetNSelections") == 0) {
    if (iface->handle_get_nselections != nullptr) {
      handled = iface->handle_get_nselections(self, invocation);
    }
  } else if (g_strcmp0(method_name, "GetSelection") == 0) {
    if (iface->handle_get_selection != nullptr) {
      gint32 selection_num;
      g_variant_get(parameters, "(i)", &selection_num);
      handled = iface->handle_get_selection(self, invocation, selection_num);
    }
  } else if (g_strcmp0(method_name, "AddSelection") == 0) {
    if (iface->handle_add_selection != nullptr) {
      gint32 start_offset;
      gint32 end_offset;
      g_variant_get(parameters, "(ii)", &start_offset, &end_offset);
      handled = iface->handle_add_selection(self, invocation, start_offset,
                                            end_offset);
    }
  } else if (g_strcmp0(method_name, "RemoveSelection") == 0) {
    if (iface->handle_remove_selection != nullptr) {
      gint32 selection_num;
      g_variant_get(parameters, "(i)", &selection_num);
      handled = iface->handle_remove_selection(self, invocation, selection_num);
    }
  } else if (g_strcmp0(method_name, "SetSelection") == 0) {
    if (iface->handle_set_selection != nullptr) {
      gint32 selection_num;
      gint32 start_offset;
      gint32 end_offset;
      g_variant_get(parameters, "(iii)", &selection_num, &start_offset,
                    &end_offset);
      handled = iface->handle_set_selection(self, invocation, selection_num,
                                            start_offset, end_offset);
    }
  } else if (g_strcmp0(method_name, "GetRangeExtents") == 0) {
    if (iface->handle_get_range_extents != nullptr) {
      gint32 start_offset;
      gint32 end_offset;
      guint32 coord_type;
      g_variant_get(parameters, "(iiu)", &start_offset, &end_offset,
                    &coord_type);
      handled = iface->handle_get_range_extents(self, invocation, start_offset,
                                                end_offset, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetBoundedRanges") == 0) {
    if (iface->handle_get_bounded_ranges != nullptr) {
      gint32 x;
      gint32 y;
      gint32 width;
      gint32 height;
      guint32 coord_type;
      guint32 x_clip_type;
      guint32 y_clip_type;
      g_variant_get(parameters, "(iiiiuuu)", &x, &y, &width, &height,
                    &coord_type, &x_clip_type, &y_clip_type);
      handled = iface->handle_get_bounded_ranges(self, invocation, x, y, width,
                                                 height, coord_type,
                                                 x_clip_type, y_clip_type);
    }
  } else if (g_strcmp0(method_name, "GetAttributeRun") == 0) {
    if (iface->handle_get_attribute_run != nullptr) {
      gint32 offset;
      gboolean include_defaults;
      g_variant_get(parameters, "(ib)", &offset, &include_defaults);
      handled = iface->handle_get_attribute_run(self, invocation, offset,
                                                include_defaults);
    }
  } else if (g_strcmp0(method_name, "GetDefaultAttributeSet") == 0) {
    if (iface->handle_get_default_attribute_set != nullptr) {
      handled = iface->handle_get_default_attribute_set(self, invocation);
    }
  } else if (g_strcmp0(method_name, "ScrollSubstringTo") == 0) {
    if (iface->handle_scroll_substring_to != nullptr) {
      gint32 start_offset;
      gint32 end_offset;
      guint32 type;
      g_variant_get(parameters, "(iiu)", &start_offset, &end_offset, &type);
      handled = iface->handle_scroll_substring_to(
          self, invocation, start_offset, end_offset, type);
    }
  } else if (g_strcmp0(method_name, "ScrollSubstringToPoint") == 0) {
    if (iface->handle_scroll_substring_to_point != nullptr) {
      gint32 start_offset;
      gint32 end_offset;
      guint32 coord_type;
      gint32 x;
      gint32 y;
      g_variant_get(parameters, "(iiuii)", &start_offset, &end_offset,
                    &coord_type, &x, &y);
      handled = iface->handle_scroll_substring_to_point(
          self, invocation, start_offset, end_offset, coord_type, x, y);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_text_get_property(GDBusConnection* connection,
                                                 const gchar* sender,
                                                 const gchar* object_path,
                                                 const gchar* interface_name,
                                                 const gchar* property_name,
                                                 GError** error,
                                                 gpointer user_data) {
  FlAtspiDBusText* self = FL_ATSPI_DBUS_TEXT(user_data);
  FlAtspiDBusTextInterface* iface = FL_ATSPI_DBUS_TEXT_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "CharacterCount") == 0 &&
      iface->get_character_count != nullptr) {
    return g_variant_new("i", iface->get_character_count(self));
  } else if (g_strcmp0(property_name, "CaretOffset") == 0 &&
             iface->get_caret_offset != nullptr) {
    return g_variant_new("i", iface->get_caret_offset(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_text_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_text_method_call,
      .get_property = fl_atspi_dbus_text_get_property,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_text_register(FlAtspiDBusText* self,
                                  GDBusConnection* connection,
                                  const gchar* object_path,
                                  GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_text_interface_info(),
      fl_atspi_dbus_text_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_text_complete_get_string_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(sii)", text, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_text(FlAtspiDBusText* self,
                                          GDBusMethodInvocation* invocation,
                                          const gchar* text) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", text));
}

void fl_atspi_dbus_text_complete_set_caret_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_text_complete_get_text_before_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(sii)", text, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_text_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(sii)", text, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_text_after_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* text,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(sii)", text, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_character_at_offset(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 character) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(i)", character));
}

void fl_atspi_dbus_text_complete_get_attribute_value(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    const gchar* value) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", value));
}

void fl_atspi_dbus_text_complete_get_attributes(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation,
      g_variant_new("(@a{ss}ii)", attributes, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_default_attributes(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a{ss})", attributes));
}

void fl_atspi_dbus_text_complete_get_character_extents(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    gint32 width,
    gint32 height) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(iiii)", x, y, width, height));
}

void fl_atspi_dbus_text_complete_get_offset_at_point(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 offset) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(i)", offset));
}

void fl_atspi_dbus_text_complete_get_nselections(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 n_selections) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(i)", n_selections));
}

void fl_atspi_dbus_text_complete_get_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(ii)", start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_add_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_text_complete_remove_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_text_complete_set_selection(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_text_complete_get_range_extents(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y,
    gint32 width,
    gint32 height) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(iiii)", x, y, width, height));
}

void fl_atspi_dbus_text_complete_get_bounded_ranges(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* ranges) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a(iisv))", ranges));
}

void fl_atspi_dbus_text_complete_get_attribute_run(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes,
    gint32 start_offset,
    gint32 end_offset) {
  g_dbus_method_invocation_return_value(
      invocation,
      g_variant_new("(@a{ss}ii)", attributes, start_offset, end_offset));
}

void fl_atspi_dbus_text_complete_get_default_attribute_set(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    GVariant* attributes) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@a{ss})", attributes));
}

void fl_atspi_dbus_text_complete_scroll_substring_to(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_text_complete_scroll_substring_to_point(
    FlAtspiDBusText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

G_DEFINE_INTERFACE(FlAtspiDBusEditableText,
                   fl_atspi_dbus_editable_text,
                   G_TYPE_OBJECT)

static void fl_atspi_dbus_editable_text_default_init(
    FlAtspiDBusEditableTextInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_editable_text_interface_info() {
  return lookup_interface_info("org.a11y.atspi.EditableText");
}

static void fl_atspi_dbus_editable_text_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
  FlAtspiDBusEditableText* self = FL_ATSPI_DBUS_EDITABLE_TEXT(user_data);
  FlAtspiDBusEditableTextInterface* iface =
      FL_ATSPI_DBUS_EDITABLE_TEXT_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "SetTextContents") == 0) {
    if (iface->handle_set_text_contents != nullptr) {
      const gchar* new_contents;
      g_variant_get(parameters, "(&s)", &new_contents);
      handled = iface->handle_set_text_contents(self, invocation, new_contents);
    }
  } else if (g_strcmp0(method_name, "InsertText") == 0) {
    if (iface->handle_insert_text != nullptr) {
      gint32 position;
      const gchar* text;
      gint32 length;
      g_variant_get(parameters, "(i&si)", &position, &text, &length);
      handled =
          iface->handle_insert_text(self, invocation, position, text, length);
    }
  } else if (g_strcmp0(method_name, "CopyText") == 0) {
    if (iface->handle_copy_text != nullptr) {
      gint32 start_pos;
      gint32 end_pos;
      g_variant_get(parameters, "(ii)", &start_pos, &end_pos);
      handled = iface->handle_copy_text(self, invocation, start_pos, end_pos);
    }
  } else if (g_strcmp0(method_name, "CutText") == 0) {
    if (iface->handle_cut_text != nullptr) {
      gint32 start_pos;
      gint32 end_pos;
      g_variant_get(parameters, "(ii)", &start_pos, &end_pos);
      handled = iface->handle_cut_text(self, invocation, start_pos, end_pos);
    }
  } else if (g_strcmp0(method_name, "DeleteText") == 0) {
    if (iface->handle_delete_text != nullptr) {
      gint32 start_pos;
      gint32 end_pos;
      g_variant_get(parameters, "(ii)", &start_pos, &end_pos);
      handled = iface->handle_delete_text(self, invocation, start_pos, end_pos);
    }
  } else if (g_strcmp0(method_name, "PasteText") == 0) {
    if (iface->handle_paste_text != nullptr) {
      gint32 position;
      g_variant_get(parameters, "(i)", &position);
      handled = iface->handle_paste_text(self, invocation, position);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static const GDBusInterfaceVTable*
fl_atspi_dbus_editable_text_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_editable_text_method_call,
      .get_property = nullptr,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_editable_text_register(FlAtspiDBusEditableText* self,
                                           GDBusConnection* connection,
                                           const gchar* object_path,
                                           GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_editable_text_interface_info(),
      fl_atspi_dbus_editable_text_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_editable_text_complete_set_text_contents(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_editable_text_complete_insert_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_editable_text_complete_copy_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

void fl_atspi_dbus_editable_text_complete_cut_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_editable_text_complete_delete_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_editable_text_complete_paste_text(
    FlAtspiDBusEditableText* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

G_DEFINE_INTERFACE(FlAtspiDBusSelection, fl_atspi_dbus_selection, G_TYPE_OBJECT)

static void fl_atspi_dbus_selection_default_init(
    FlAtspiDBusSelectionInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_selection_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Selection");
}

static void fl_atspi_dbus_selection_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
  FlAtspiDBusSelection* self = FL_ATSPI_DBUS_SELECTION(user_data);
  FlAtspiDBusSelectionInterface* iface =
      FL_ATSPI_DBUS_SELECTION_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetSelectedChild") == 0) {
    if (iface->handle_get_selected_child != nullptr) {
      gint32 selected_child_index;
      g_variant_get(parameters, "(i)", &selected_child_index);
      handled = iface->handle_get_selected_child(self, invocation,
                                                 selected_child_index);
    }
  } else if (g_strcmp0(method_name, "SelectChild") == 0) {
    if (iface->handle_select_child != nullptr) {
      gint32 child_index;
      g_variant_get(parameters, "(i)", &child_index);
      handled = iface->handle_select_child(self, invocation, child_index);
    }
  } else if (g_strcmp0(method_name, "DeselectSelectedChild") == 0) {
    if (iface->handle_deselect_selected_child != nullptr) {
      gint32 selected_child_index;
      g_variant_get(parameters, "(i)", &selected_child_index);
      handled = iface->handle_deselect_selected_child(self, invocation,
                                                      selected_child_index);
    }
  } else if (g_strcmp0(method_name, "IsChildSelected") == 0) {
    if (iface->handle_is_child_selected != nullptr) {
      gint32 child_index;
      g_variant_get(parameters, "(i)", &child_index);
      handled = iface->handle_is_child_selected(self, invocation, child_index);
    }
  } else if (g_strcmp0(method_name, "SelectAll") == 0) {
    if (iface->handle_select_all != nullptr) {
      handled = iface->handle_select_all(self, invocation);
    }
  } else if (g_strcmp0(method_name, "ClearSelection") == 0) {
    if (iface->handle_clear_selection != nullptr) {
      handled = iface->handle_clear_selection(self, invocation);
    }
  } else if (g_strcmp0(method_name, "DeselectChild") == 0) {
    if (iface->handle_deselect_child != nullptr) {
      gint32 child_index;
      g_variant_get(parameters, "(i)", &child_index);
      handled = iface->handle_deselect_child(self, invocation, child_index);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_selection_get_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GError** error,
    gpointer user_data) {
  FlAtspiDBusSelection* self = FL_ATSPI_DBUS_SELECTION(user_data);
  FlAtspiDBusSelectionInterface* iface =
      FL_ATSPI_DBUS_SELECTION_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "NSelectedChildren") == 0 &&
      iface->get_nselected_children != nullptr) {
    return g_variant_new("i", iface->get_nselected_children(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_selection_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_selection_method_call,
      .get_property = fl_atspi_dbus_selection_get_property,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_selection_register(FlAtspiDBusSelection* self,
                                       GDBusConnection* connection,
                                       const gchar* object_path,
                                       GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_selection_interface_info(),
      fl_atspi_dbus_selection_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_selection_complete_get_selected_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    GVariant* child) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(so))", child));
}

void fl_atspi_dbus_selection_complete_select_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_selection_complete_deselect_selected_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_selection_complete_is_child_selected(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_selection_complete_select_all(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_selection_complete_clear_selection(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

void fl_atspi_dbus_selection_complete_deselect_child(
    FlAtspiDBusSelection* self,
    GDBusMethodInvocation* invocation,
    gboolean result) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(b)", result));
}

G_DEFINE_INTERFACE(FlAtspiDBusImage, fl_atspi_dbus_image, G_TYPE_OBJECT)

static void fl_atspi_dbus_image_default_init(FlAtspiDBusImageInterface* iface) {
}

GDBusInterfaceInfo* fl_atspi_dbus_image_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Image");
}

static void fl_atspi_dbus_image_method_call(GDBusConnection* connection,
                                            const gchar* sender,
                                            const gchar* object_path,
                                            const gchar* interface_name,
                                            const gchar* method_name,
                                            GVariant* parameters,
                                            GDBusMethodInvocation* invocation,
                                            gpointer user_data) {
  FlAtspiDBusImage* self = FL_ATSPI_DBUS_IMAGE(user_data);
  FlAtspiDBusImageInterface* iface = FL_ATSPI_DBUS_IMAGE_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetImageExtents") == 0) {
    if (iface->handle_get_image_extents != nullptr) {
      guint32 coord_type;
      g_variant_get(parameters, "(u)", &coord_type);
      handled = iface->handle_get_image_extents(self, invocation, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetImagePosition") == 0) {
    if (iface->handle_get_image_position != nullptr) {
      guint32 coord_type;
      g_variant_get(parameters, "(u)", &coord_type);
      handled = iface->handle_get_image_position(self, invocation, coord_type);
    }
  } else if (g_strcmp0(method_name, "GetImageSize") == 0) {
    if (iface->handle_get_image_size != nullptr) {
      handled = iface->handle_get_image_size(self, invocation);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static GVariant* fl_atspi_dbus_image_get_property(GDBusConnection* connection,
                                                  const gchar* sender,
                                                  const gchar* object_path,
                                                  const gchar* interface_name,
                                                  const gchar* property_name,
                                                  GError** error,
                                                  gpointer user_data) {
  FlAtspiDBusImage* self = FL_ATSPI_DBUS_IMAGE(user_data);
  FlAtspiDBusImageInterface* iface = FL_ATSPI_DBUS_IMAGE_GET_IFACE(self);
  GVariant* value = nullptr;
  if (g_strcmp0(property_name, "ImageDescription") == 0 &&
      iface->get_image_description != nullptr) {
    return take_string(iface->get_image_description(self));
  } else if (g_strcmp0(property_name, "ImageLocale") == 0 &&
             iface->get_image_locale != nullptr) {
    return take_string(iface->get_image_locale(self));
  }
  if (value == nullptr) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Property %s.%s is not implemented", interface_name,
                property_name);
  }
  return value;
}

static const GDBusInterfaceVTable* fl_atspi_dbus_image_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_image_method_call,
      .get_property = fl_atspi_dbus_image_get_property,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_image_register(FlAtspiDBusImage* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_image_interface_info(),
      fl_atspi_dbus_image_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_image_complete_get_image_extents(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    GVariant* extents) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(@(iiii))", extents));
}

void fl_atspi_dbus_image_complete_get_image_position(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    gint32 x,
    gint32 y) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(ii)", x, y));
}

void fl_atspi_dbus_image_complete_get_image_size(
    FlAtspiDBusImage* self,
    GDBusMethodInvocation* invocation,
    gint32 width,
    gint32 height) {
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(ii)", width, height));
}

G_DEFINE_INTERFACE(FlAtspiDBusSocket, fl_atspi_dbus_socket, G_TYPE_OBJECT)

static void fl_atspi_dbus_socket_default_init(
    FlAtspiDBusSocketInterface* iface) {}

GDBusInterfaceInfo* fl_atspi_dbus_socket_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Socket");
}

static void fl_atspi_dbus_socket_method_call(GDBusConnection* connection,
                                             const gchar* sender,
                                             const gchar* object_path,
                                             const gchar* interface_name,
                                             const gchar* method_name,
                                             GVariant* parameters,
                                             GDBusMethodInvocation* invocation,
                                             gpointer user_data) {
  FlAtspiDBusSocket* self = FL_ATSPI_DBUS_SOCKET(user_data);
  FlAtspiDBusSocketInterface* iface = FL_ATSPI_DBUS_SOCKET_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "Embedded") == 0) {
    if (iface->handle_embedded != nullptr) {
      const gchar* path;
      g_variant_get(parameters, "(&s)", &path);
      handled = iface->handle_embedded(self, invocation, path);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static const GDBusInterfaceVTable* fl_atspi_dbus_socket_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_socket_method_call,
      .get_property = nullptr,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_socket_register(FlAtspiDBusSocket* self,
                                    GDBusConnection* connection,
                                    const gchar* object_path,
                                    GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_socket_interface_info(),
      fl_atspi_dbus_socket_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_socket_complete_embedded(FlAtspiDBusSocket* self,
                                            GDBusMethodInvocation* invocation) {
  g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

G_DEFINE_INTERFACE(FlAtspiDBusCache, fl_atspi_dbus_cache, G_TYPE_OBJECT)

static void fl_atspi_dbus_cache_default_init(FlAtspiDBusCacheInterface* iface) {
}

GDBusInterfaceInfo* fl_atspi_dbus_cache_interface_info() {
  return lookup_interface_info("org.a11y.atspi.Cache");
}

static void fl_atspi_dbus_cache_method_call(GDBusConnection* connection,
                                            const gchar* sender,
                                            const gchar* object_path,
                                            const gchar* interface_name,
                                            const gchar* method_name,
                                            GVariant* parameters,
                                            GDBusMethodInvocation* invocation,
                                            gpointer user_data) {
  FlAtspiDBusCache* self = FL_ATSPI_DBUS_CACHE(user_data);
  FlAtspiDBusCacheInterface* iface = FL_ATSPI_DBUS_CACHE_GET_IFACE(self);
  gboolean handled = FALSE;
  if (g_strcmp0(method_name, "GetItems") == 0) {
    if (iface->handle_get_items != nullptr) {
      handled = iface->handle_get_items(self, invocation);
    }
  }
  if (!handled) {
    g_dbus_method_invocation_return_error(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
        "Method %s.%s is not implemented", interface_name, method_name);
  }
}

static const GDBusInterfaceVTable* fl_atspi_dbus_cache_interface_vtable() {
  static const GDBusInterfaceVTable vtable = {
      .method_call = fl_atspi_dbus_cache_method_call,
      .get_property = nullptr,
      .set_property = nullptr,
  };
  return &vtable;
}

guint fl_atspi_dbus_cache_register(FlAtspiDBusCache* self,
                                   GDBusConnection* connection,
                                   const gchar* object_path,
                                   GError** error) {
  return g_dbus_connection_register_object(
      connection, object_path, fl_atspi_dbus_cache_interface_info(),
      fl_atspi_dbus_cache_interface_vtable(), self, nullptr, error);
}

void fl_atspi_dbus_cache_complete_get_items(FlAtspiDBusCache* self,
                                            GDBusMethodInvocation* invocation,
                                            GVariant* nodes) {
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(@a((so)(so)(so)iiassusau))", nodes));
}

void fl_atspi_dbus_event_object_emit_property_change(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object", "PropertyChange",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_state_changed(GDBusConnection* connection,
                                                   const gchar* object_path,
                                                   const gchar* detail,
                                                   gint32 detail1,
                                                   gint32 detail2,
                                                   GVariant* any_data,
                                                   GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object", "StateChanged",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_children_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object",
                                "ChildrenChanged",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_selection_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object",
                                "SelectionChanged",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_announcement(GDBusConnection* connection,
                                                  const gchar* object_path,
                                                  const gchar* detail,
                                                  gint32 detail1,
                                                  gint32 detail2,
                                                  GVariant* any_data,
                                                  GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object", "Announcement",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_text_selection_changed(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object",
                                "TextSelectionChanged",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_text_changed(GDBusConnection* connection,
                                                  const gchar* object_path,
                                                  const gchar* detail,
                                                  gint32 detail1,
                                                  gint32 detail2,
                                                  GVariant* any_data,
                                                  GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object", "TextChanged",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}

void fl_atspi_dbus_event_object_emit_text_caret_moved(
    GDBusConnection* connection,
    const gchar* object_path,
    const gchar* detail,
    gint32 detail1,
    gint32 detail2,
    GVariant* any_data,
    GVariant* properties) {
  g_dbus_connection_emit_signal(connection, nullptr, object_path,
                                "org.a11y.atspi.Event.Object", "TextCaretMoved",
                                g_variant_new("(siiv@a{sv})", detail, detail1,
                                              detail2, any_data, properties),
                                nullptr);
}
