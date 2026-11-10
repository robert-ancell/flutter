// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_application.h"

#include "flutter/shell/platform/linux/fl_atspi_application_private.h"
#include "flutter/shell/platform/linux/fl_atspi_dbus.h"
#include "flutter/shell/platform/linux/fl_atspi_object.h"

static constexpr char kBusName[] = "org.a11y.Bus";
static constexpr char kBusPath[] = "/org/a11y/bus";

static constexpr char kRegistryName[] = "org.a11y.atspi.Registry";
static constexpr char kRegistryPath[] = "/org/a11y/atspi/registry";

static constexpr char kRootPath[] = "/org/a11y/atspi/accessible/root";
static constexpr char kCachePath[] = "/org/a11y/atspi/cache";
static constexpr char kNullPath[] = "/org/a11y/atspi/null";
// Objects other than the root are given paths under this path when first
// referenced.
static constexpr char kObjectPath[] = "/org/flutter/a11y";

// A window and the socket it is embedded in.
typedef struct {
  FlAtspiAccessible* accessible;

  // The AtkSocket to embed this window into.
  GWeakRef socket;
} Window;

// An event a listener has registered for.
typedef struct {
  gchar* bus;
  gchar* event;
} RegisteredEvent;

struct _FlAtspiApplication {
  GObject parent_instance;

  // The windows, the children of the application.
  GPtrArray* windows;

  // The exported objects, keyed by the accessible they export. Objects are
  // exported when first referenced and unexported when the accessible is
  // destroyed.
  GHashTable* objects;
  guint64 next_object_id;

  GCancellable* cancellable;

  // Cancels connecting to the accessibility bus, set while connecting.
  GCancellable* start_cancellable;

  GDBusConnection* connection;
  guint cache_registration_id;
  guint registered_subscription_id;
  guint deregistered_subscription_id;
  guint name_owner_subscription_id;
  gulong closed_handler_id;

  // The events listeners have registered for. Until the registry reports
  // them, all events are sent.
  gboolean events_known;
  GPtrArray* registered_events;
};

static void fl_atspi_application_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface);
static void fl_atspi_application_dbus_cache_iface_init(
    FlAtspiDBusCacheInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlAtspiApplication,
    fl_atspi_application,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(fl_atspi_accessible_get_type(),
                          fl_atspi_application_atspi_accessible_iface_init)
        G_IMPLEMENT_INTERFACE(fl_atspi_dbus_cache_get_type(),
                              fl_atspi_application_dbus_cache_iface_init))

static void window_free(Window* window) {
  g_weak_ref_clear(&window->socket);
  g_free(window);
}

static RegisteredEvent* registered_event_new(const gchar* bus,
                                             const gchar* event) {
  RegisteredEvent* entry = g_new0(RegisteredEvent, 1);
  entry->bus = g_strdup(bus);
  entry->event = g_strdup(event);
  return entry;
}

static void registered_event_free(RegisteredEvent* event) {
  g_free(event->bus);
  g_free(event->event);
  g_free(event);
}

static const gchar* get_unique_name(FlAtspiApplication* self) {
  const gchar* name = self->connection != nullptr
                          ? g_dbus_connection_get_unique_name(self->connection)
                          : nullptr;
  return name != nullptr ? name : "";
}

// TRUE if connected to a message bus rather than a peer.
static gboolean is_message_bus(FlAtspiApplication* self) {
  return g_dbus_connection_get_unique_name(self->connection) != nullptr;
}

// Returns the name of a state used in StateChanged events, e.g. "read-only".
static const gchar* get_state_name(FlAtspiState state) {
  switch (state) {
    case FL_ATSPI_STATE_CHECKED:
      return "checked";
    case FL_ATSPI_STATE_COLLAPSED:
      return "collapsed";
    case FL_ATSPI_STATE_EDITABLE:
      return "editable";
    case FL_ATSPI_STATE_ENABLED:
      return "enabled";
    case FL_ATSPI_STATE_EXPANDABLE:
      return "expandable";
    case FL_ATSPI_STATE_EXPANDED:
      return "expanded";
    case FL_ATSPI_STATE_FOCUSABLE:
      return "focusable";
    case FL_ATSPI_STATE_FOCUSED:
      return "focused";
    case FL_ATSPI_STATE_HAS_TOOLTIP:
      return "has-tooltip";
    case FL_ATSPI_STATE_MULTI_LINE:
      return "multi-line";
    case FL_ATSPI_STATE_SELECTABLE:
      return "selectable";
    case FL_ATSPI_STATE_SELECTED:
      return "selected";
    case FL_ATSPI_STATE_SENSITIVE:
      return "sensitive";
    case FL_ATSPI_STATE_SHOWING:
      return "showing";
    case FL_ATSPI_STATE_SINGLE_LINE:
      return "single-line";
    case FL_ATSPI_STATE_VISIBLE:
      return "visible";
    case FL_ATSPI_STATE_INDETERMINATE:
      return "indeterminate";
    case FL_ATSPI_STATE_REQUIRED:
      return "required";
    case FL_ATSPI_STATE_SELECTABLE_TEXT:
      return "selectable-text";
    case FL_ATSPI_STATE_CHECKABLE:
      return "checkable";
    case FL_ATSPI_STATE_READ_ONLY:
      return "read-only";
  }
  return "invalid";
}

// Converts an event name part to the format listeners register with, e.g.
// "state-changed" to "StateChanged".
static gchar* to_event_format(const gchar* name) {
  GString* result = g_string_new("");
  gboolean upper = TRUE;
  for (const gchar* c = name; *c != '\0'; c++) {
    if (*c == '-') {
      upper = TRUE;
    } else if (upper) {
      g_string_append_c(result, g_ascii_toupper(*c));
      upper = FALSE;
    } else {
      g_string_append_c(result, *c);
    }
  }
  return g_string_free(result, FALSE);
}

static void object_destroyed_cb(gpointer user_data, GObject* object) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  g_hash_table_remove(self->objects, object);
}

// Returns the D-Bus object path of |accessible|, exporting it if it hasn't been
// already.
static const gchar* get_object_path(FlAtspiApplication* self,
                                    FlAtspiAccessible* accessible) {
  FlAtspiObject* object = static_cast<FlAtspiObject*>(
      g_hash_table_lookup(self->objects, accessible));
  if (object != nullptr) {
    return fl_atspi_object_get_path(object);
  }
  g_autofree gchar* path = nullptr;
  if (accessible == FL_ATSPI_ACCESSIBLE(self)) {
    path = g_strdup(kRootPath);
  } else {
    path = g_strdup_printf("%s/%" G_GUINT64_FORMAT, kObjectPath,
                           self->next_object_id++);
    g_object_weak_ref(G_OBJECT(accessible), object_destroyed_cb, self);
  }
  object = fl_atspi_object_new(self, accessible, self->connection, path);
  g_hash_table_insert(self->objects, accessible, object);
  return fl_atspi_object_get_path(object);
}

// Unexports all objects.
static void remove_objects(FlAtspiApplication* self) {
  GHashTableIter iter;
  gpointer accessible;
  g_hash_table_iter_init(&iter, self->objects);
  while (g_hash_table_iter_next(&iter, &accessible, nullptr)) {
    if (accessible != self) {
      g_object_weak_unref(G_OBJECT(accessible), object_destroyed_cb, self);
    }
  }
  g_hash_table_remove_all(self->objects);
}

static Window* lookup_window(FlAtspiApplication* self,
                             FlAtspiAccessible* accessible) {
  for (guint i = 0; i < self->windows->len; i++) {
    Window* window = static_cast<Window*>(g_ptr_array_index(self->windows, i));
    if (window->accessible == accessible) {
      return window;
    }
  }
  return nullptr;
}

static GVariant* make_reference(FlAtspiApplication* self,
                                FlAtspiAccessible* object) {
  return g_variant_new(
      "(so)", get_unique_name(self),
      object != nullptr ? get_object_path(self, object) : kNullPath);
}

// Embeds |window| into its socket in the GTK accessibility tree.
static void embed_window(FlAtspiApplication* self, Window* window) {
  if (self->connection == nullptr) {
    return;
  }
  g_autoptr(AtkSocket) socket = ATK_SOCKET(g_weak_ref_get(&window->socket));
  if (socket == nullptr) {
    return;
  }
  g_autofree gchar* plug_id =
      g_strdup_printf("%s:%s", get_unique_name(self),
                      get_object_path(self, window->accessible));

  // AtkSocket and AtkPlug are designed for use between processes. at-spi2-atk
  // replaces ref_state_set with a blocking D-Bus call to the plug, which can't
  // be answered as the plug is in this process. Keep our implementation.
  AtkObjectClass* socket_class = ATK_OBJECT_GET_CLASS(socket);
  auto ref_state_set = socket_class->ref_state_set;
  atk_socket_embed(socket, plug_id);
  socket_class->ref_state_set = ref_state_set;
}

static void disconnect(FlAtspiApplication* self) {
  if (self->connection == nullptr) {
    return;
  }
  g_signal_handler_disconnect(self->connection, self->closed_handler_id);
  self->closed_handler_id = 0;
  remove_objects(self);
  if (self->cache_registration_id != 0) {
    g_dbus_connection_unregister_object(self->connection,
                                        self->cache_registration_id);
    self->cache_registration_id = 0;
  }
  guint* subscription_ids[] = {&self->registered_subscription_id,
                               &self->deregistered_subscription_id,
                               &self->name_owner_subscription_id};
  for (guint* id : subscription_ids) {
    if (*id != 0) {
      g_dbus_connection_signal_unsubscribe(self->connection, *id);
      *id = 0;
    }
  }
  g_clear_object(&self->connection);
  self->events_known = FALSE;
  g_ptr_array_set_size(self->registered_events, 0);
}

static void connection_closed_cb(GDBusConnection* connection,
                                 gboolean remote_peer_vanished,
                                 GError* error,
                                 gpointer user_data) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  disconnect(self);
}

static void registered_events_cb(GObject* object,
                                 GAsyncResult* result,
                                 gpointer user_data) {
  g_autoptr(GVariant) events = nullptr;
  if (!fl_atspi_dbus_registry_call_get_registered_events_finish(
          G_DBUS_CONNECTION(object), result, &events, nullptr)) {
    // Cancelled or failed, in which case all events are sent.
    return;
  }
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  if (self->connection != G_DBUS_CONNECTION(object)) {
    return;
  }
  g_ptr_array_set_size(self->registered_events, 0);
  GVariantIter iter;
  g_variant_iter_init(&iter, events);
  const gchar* bus;
  const gchar* event;
  while (g_variant_iter_next(&iter, "(&s&s)", &bus, &event)) {
    g_ptr_array_add(self->registered_events, registered_event_new(bus, event));
  }
  self->events_known = TRUE;
}

static void request_registered_events(FlAtspiApplication* self) {
  fl_atspi_dbus_registry_call_get_registered_events(
      self->connection, is_message_bus(self) ? kRegistryName : nullptr,
      kRegistryPath, self->cancellable, registered_events_cb, self);
}

static void event_listener_registered_cb(const gchar* bus,
                                         const gchar* event,
                                         const gchar* const* properties,
                                         gpointer user_data) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  g_ptr_array_add(self->registered_events, registered_event_new(bus, event));
}

static void event_listener_deregistered_cb(const gchar* bus,
                                           const gchar* event,
                                           gpointer user_data) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  for (guint i = 0; i < self->registered_events->len; i++) {
    RegisteredEvent* entry = static_cast<RegisteredEvent*>(
        g_ptr_array_index(self->registered_events, i));
    if (g_strcmp0(entry->bus, bus) == 0 &&
        g_strcmp0(entry->event, event) == 0) {
      g_ptr_array_remove_index(self->registered_events, i);
      break;
    }
  }
}

static void registry_name_owner_changed_cb(GDBusConnection* connection,
                                           const gchar* sender_name,
                                           const gchar* object_path,
                                           const gchar* interface_name,
                                           const gchar* signal_name,
                                           GVariant* parameters,
                                           gpointer user_data) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  const gchar* new_owner;
  g_variant_get(parameters, "(&s&s&s)", nullptr, nullptr, &new_owner);
  if (new_owner[0] != '\0') {
    // The registry restarted.
    request_registered_events(self);
  }
}

static void set_connection(FlAtspiApplication* self,
                           GDBusConnection* connection);

static void connection_ready_cb(GObject* object,
                                GAsyncResult* result,
                                gpointer user_data) {
  g_autoptr(GError) error = nullptr;
  g_autoptr(GDBusConnection) connection =
      g_dbus_connection_new_for_address_finish(result, &error);
  if (connection == nullptr) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_warning("Failed to connect to the accessibility bus: %s",
                error->message);
      g_clear_object(&FL_ATSPI_APPLICATION(user_data)->start_cancellable);
    }
    return;
  }
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  g_clear_object(&self->start_cancellable);
  set_connection(self, connection);
}

static void connect_accessibility_bus(FlAtspiApplication* self,
                                      const gchar* address) {
  g_dbus_connection_new_for_address(
      address,
      static_cast<GDBusConnectionFlags>(
          G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
          G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
      nullptr, self->start_cancellable, connection_ready_cb, self);
}

static void address_ready_cb(GObject* object,
                             GAsyncResult* result,
                             gpointer user_data) {
  g_autoptr(GError) error = nullptr;
  g_autofree gchar* address = nullptr;
  if (!fl_atspi_dbus_bus_call_get_address_finish(G_DBUS_CONNECTION(object),
                                                 result, &address, &error)) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug("Failed to get accessibility bus address: %s", error->message);
      g_clear_object(&FL_ATSPI_APPLICATION(user_data)->start_cancellable);
    }
    return;
  }
  connect_accessibility_bus(FL_ATSPI_APPLICATION(user_data), address);
}

static void session_bus_ready_cb(GObject* object,
                                 GAsyncResult* result,
                                 gpointer user_data) {
  g_autoptr(GError) error = nullptr;
  g_autoptr(GDBusConnection) session = g_bus_get_finish(result, &error);
  if (session == nullptr) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug("Failed to connect to session bus: %s", error->message);
      g_clear_object(&FL_ATSPI_APPLICATION(user_data)->start_cancellable);
    }
    return;
  }
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(user_data);
  fl_atspi_dbus_bus_call_get_address(session, kBusName, kBusPath,
                                     self->start_cancellable, address_ready_cb,
                                     self);
}

// TRUE if a listener wants events of this type.
static gboolean is_event_needed(FlAtspiApplication* self,
                                const gchar* member,
                                const gchar* detail) {
  if (!self->events_known) {
    return TRUE;
  }
  // Events that keep clients' caches up to date are always needed.
  if (g_strcmp0(member, "ChildrenChanged") == 0 ||
      g_strcmp0(member, "StateChanged") == 0 ||
      (g_strcmp0(member, "PropertyChange") == 0 &&
       (g_strcmp0(detail, "accessible-name") == 0 ||
        g_strcmp0(detail, "accessible-description") == 0 ||
        g_strcmp0(detail, "accessible-parent") == 0 ||
        g_strcmp0(detail, "accessible-role") == 0))) {
    return TRUE;
  }
  g_autofree gchar* event_detail = to_event_format(detail);
  const gchar* needle[3] = {"Object", member, event_detail};
  for (guint i = 0; i < self->registered_events->len; i++) {
    RegisteredEvent* entry = static_cast<RegisteredEvent*>(
        g_ptr_array_index(self->registered_events, i));
    g_auto(GStrv) parts = g_strsplit(entry->event, ":", 3);
    gboolean matches = TRUE;
    for (int j = 0; parts[j] != nullptr && parts[j][0] != '\0'; j++) {
      if (g_strcmp0(parts[j], needle[j]) != 0) {
        matches = FALSE;
        break;
      }
    }
    if (matches) {
      return TRUE;
    }
  }
  return FALSE;
}

typedef void (*EmitFunction)(GDBusConnection* connection,
                             const gchar* object_path,
                             const gchar* detail,
                             gint32 detail1,
                             gint32 detail2,
                             GVariant* any_data,
                             GVariant* properties);

// Emits an Object event |member| from |source| if a listener wants it.
static void emit(FlAtspiApplication* self,
                 EmitFunction emit_function,
                 const gchar* member,
                 FlAtspiAccessible* source,
                 const gchar* detail,
                 gint32 detail1,
                 gint32 detail2,
                 GVariant* data) {
  if (self->connection == nullptr || source == nullptr ||
      !is_event_needed(self, member, detail)) {
    return;
  }
  emit_function(self->connection, get_object_path(self, source), detail,
                detail1, detail2,
                data != nullptr ? data : g_variant_new_int32(0),
                g_variant_new("a{sv}", nullptr));
}

static FlAtspiAccessible* fl_atspi_application_get_parent(
    FlAtspiAccessible* accessible) {
  return nullptr;
}

static gsize fl_atspi_application_get_n_children(
    FlAtspiAccessible* accessible) {
  return FL_ATSPI_APPLICATION(accessible)->windows->len;
}

static FlAtspiAccessible* fl_atspi_application_get_child(
    FlAtspiAccessible* accessible,
    gsize index) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(accessible);
  if (index >= self->windows->len) {
    return nullptr;
  }
  return static_cast<Window*>(g_ptr_array_index(self->windows, index))
      ->accessible;
}

static FlAtspiRole fl_atspi_application_get_role(
    FlAtspiAccessible* accessible) {
  return FL_ATSPI_ROLE_APPLICATION;
}

static gchar* fl_atspi_application_get_name(FlAtspiAccessible* accessible) {
  const gchar* name = g_get_prgname();
  return g_strdup(name != nullptr ? name : "");
}

static guint64 fl_atspi_application_get_states(FlAtspiAccessible* accessible) {
  return 0;
}

static void fl_atspi_application_atspi_accessible_iface_init(
    FlAtspiAccessibleInterface* iface) {
  iface->get_parent = fl_atspi_application_get_parent;
  iface->get_n_children = fl_atspi_application_get_n_children;
  iface->get_child = fl_atspi_application_get_child;
  iface->get_role = fl_atspi_application_get_role;
  iface->get_name = fl_atspi_application_get_name;
  iface->get_states = fl_atspi_application_get_states;
}

// org.a11y.atspi.Cache

static gboolean cache_handle_get_items(FlAtspiDBusCache* dbus,
                                       GDBusMethodInvocation* invocation) {
  // Clients fall back to querying objects when the cache is empty.
  fl_atspi_dbus_cache_complete_get_items(
      dbus, invocation, g_variant_new("a((so)(so)(so)iiassusau)", nullptr));
  return TRUE;
}

static void fl_atspi_application_dbus_cache_iface_init(
    FlAtspiDBusCacheInterface* iface) {
  iface->handle_get_items = cache_handle_get_items;
}

static void fl_atspi_application_dispose(GObject* object) {
  FlAtspiApplication* self = FL_ATSPI_APPLICATION(object);

  g_cancellable_cancel(self->cancellable);
  if (self->start_cancellable != nullptr) {
    g_cancellable_cancel(self->start_cancellable);
    g_clear_object(&self->start_cancellable);
  }
  disconnect(self);
  g_clear_object(&self->cancellable);
  g_clear_pointer(&self->windows, g_ptr_array_unref);
  g_clear_pointer(&self->objects, g_hash_table_unref);
  g_clear_pointer(&self->registered_events, g_ptr_array_unref);

  G_OBJECT_CLASS(fl_atspi_application_parent_class)->dispose(object);
}

static void fl_atspi_application_class_init(FlAtspiApplicationClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_atspi_application_dispose;
}

static void fl_atspi_application_init(FlAtspiApplication* self) {
  self->windows = g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(window_free));
  self->objects = g_hash_table_new_full(g_direct_hash, g_direct_equal, nullptr,
                                        g_object_unref);
  self->next_object_id = 1;
  self->cancellable = g_cancellable_new();
  self->registered_events = g_ptr_array_new_with_free_func(
      reinterpret_cast<GDestroyNotify>(registered_event_free));
}

FlAtspiApplication* fl_atspi_application_new() {
  return FL_ATSPI_APPLICATION(
      g_object_new(fl_atspi_application_get_type(), nullptr));
}

FlAtspiApplication* fl_atspi_application_new_with_connection(
    GDBusConnection* connection) {
  g_return_val_if_fail(G_IS_DBUS_CONNECTION(connection), nullptr);

  FlAtspiApplication* self = fl_atspi_application_new();
  set_connection(self, connection);
  return self;
}

void fl_atspi_application_start(FlAtspiApplication* self) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));

  if (self->connection != nullptr || self->start_cancellable != nullptr) {
    return;
  }
  self->start_cancellable = g_cancellable_new();

  const gchar* address = g_getenv("AT_SPI_BUS_ADDRESS");
  if (address != nullptr && address[0] != '\0') {
    connect_accessibility_bus(self, address);
    return;
  }
  g_bus_get(G_BUS_TYPE_SESSION, self->start_cancellable, session_bus_ready_cb,
            self);
}

static void set_connection(FlAtspiApplication* self,
                           GDBusConnection* connection) {
  self->connection = G_DBUS_CONNECTION(g_object_ref(connection));
  self->closed_handler_id = g_signal_connect(
      self->connection, "closed", G_CALLBACK(connection_closed_cb), self);

  get_object_path(self, FL_ATSPI_ACCESSIBLE(self));
  g_autoptr(GError) error = nullptr;
  self->cache_registration_id = fl_atspi_dbus_cache_register(
      FL_ATSPI_DBUS_CACHE(self), self->connection, kCachePath, &error);
  if (self->cache_registration_id == 0) {
    g_warning("Failed to register accessibility cache: %s", error->message);
  }

  // Signal subscriptions on a peer-to-peer connection can't specify a sender.
  gboolean is_bus = is_message_bus(self);
  const gchar* registry_name = is_bus ? kRegistryName : nullptr;
  self->registered_subscription_id =
      fl_atspi_dbus_registry_subscribe_event_listener_registered(
          self->connection, registry_name, kRegistryPath,
          event_listener_registered_cb, self);
  self->deregistered_subscription_id =
      fl_atspi_dbus_registry_subscribe_event_listener_deregistered(
          self->connection, registry_name, kRegistryPath,
          event_listener_deregistered_cb, self);
  if (is_bus) {
    self->name_owner_subscription_id = g_dbus_connection_signal_subscribe(
        self->connection, "org.freedesktop.DBus", "org.freedesktop.DBus",
        "NameOwnerChanged", "/org/freedesktop/DBus", kRegistryName,
        G_DBUS_SIGNAL_FLAGS_NONE, registry_name_owner_changed_cb, self,
        nullptr);
  }
  request_registered_events(self);

  for (guint i = 0; i < self->windows->len; i++) {
    embed_window(self,
                 static_cast<Window*>(g_ptr_array_index(self->windows, i)));
  }
}

void fl_atspi_application_add_window(FlAtspiApplication* self,
                                     FlAtspiAccessible* window) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));

  Window* entry = g_new0(Window, 1);
  entry->accessible = window;
  g_weak_ref_init(&entry->socket, nullptr);
  g_ptr_array_add(self->windows, entry);
}

void fl_atspi_application_remove_window(FlAtspiApplication* self,
                                        FlAtspiAccessible* window) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));

  Window* entry = lookup_window(self, window);
  if (entry != nullptr) {
    g_ptr_array_remove(self->windows, entry);
  }
}

void fl_atspi_application_set_window_socket(FlAtspiApplication* self,
                                            FlAtspiAccessible* window,
                                            AtkSocket* socket) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));

  Window* entry = lookup_window(self, window);
  g_return_if_fail(entry != nullptr);
  g_weak_ref_set(&entry->socket, socket);
  embed_window(self, entry);
}

void fl_atspi_application_emit_children_changed(FlAtspiApplication* self,
                                                FlAtspiAccessible* parent,
                                                gboolean added,
                                                gint index,
                                                FlAtspiAccessible* child) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  if (self->connection == nullptr) {
    return;
  }
  g_autoptr(GVariant) reference =
      g_variant_ref_sink(make_reference(self, child));
  emit(self, fl_atspi_dbus_event_object_emit_children_changed,
       "ChildrenChanged", parent, added ? "add" : "remove", index, 0,
       reference);
}

void fl_atspi_application_emit_property_changed(FlAtspiApplication* self,
                                                FlAtspiAccessible* source,
                                                FlAtspiProperty property) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  if (self->connection == nullptr) {
    return;
  }
  const gchar* detail = nullptr;
  g_autoptr(GVariant) data = nullptr;
  switch (property) {
    case FL_ATSPI_PROPERTY_NAME: {
      g_autofree gchar* name = fl_atspi_accessible_get_name(source);
      detail = "accessible-name";
      data = g_variant_new_string(name != nullptr ? name : "");
      break;
    }
    case FL_ATSPI_PROPERTY_DESCRIPTION: {
      g_autofree gchar* description =
          fl_atspi_accessible_get_description(source);
      detail = "accessible-description";
      data = g_variant_new_string(description != nullptr ? description : "");
      break;
    }
    case FL_ATSPI_PROPERTY_ROLE:
      detail = "accessible-role";
      data = g_variant_new_int32(fl_atspi_accessible_get_role(source));
      break;
    case FL_ATSPI_PROPERTY_VALUE: {
      g_autoptr(FlAtspiValue) value = fl_atspi_accessible_get_value(source);
      detail = "accessible-value";
      data = g_variant_new_int32(
          value != nullptr ? static_cast<gint32>(value->current) : 0);
      break;
    }
  }
  g_variant_ref_sink(data);
  emit(self, fl_atspi_dbus_event_object_emit_property_change, "PropertyChange",
       source, detail, 0, 0, data);
}

void fl_atspi_application_emit_state_changed(FlAtspiApplication* self,
                                             FlAtspiAccessible* source,
                                             FlAtspiState state,
                                             gboolean value) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  emit(self, fl_atspi_dbus_event_object_emit_state_changed, "StateChanged",
       source, get_state_name(state), value ? 1 : 0, 0, nullptr);
}

void fl_atspi_application_emit_text_changed(FlAtspiApplication* self,
                                            FlAtspiAccessible* source,
                                            gboolean inserted,
                                            gint offset,
                                            const gchar* text) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  g_autoptr(GVariant) data = g_variant_ref_sink(g_variant_new_string(text));
  emit(self, fl_atspi_dbus_event_object_emit_text_changed, "TextChanged",
       source, inserted ? "insert" : "delete", offset, g_utf8_strlen(text, -1),
       data);
}

void fl_atspi_application_emit_text_caret_moved(FlAtspiApplication* self,
                                                FlAtspiAccessible* source,
                                                gint offset) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  emit(self, fl_atspi_dbus_event_object_emit_text_caret_moved, "TextCaretMoved",
       source, "", offset, 0, nullptr);
}

void fl_atspi_application_emit_text_selection_changed(
    FlAtspiApplication* self,
    FlAtspiAccessible* source) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  emit(self, fl_atspi_dbus_event_object_emit_text_selection_changed,
       "TextSelectionChanged", source, "", 0, 0, nullptr);
}

void fl_atspi_application_emit_selection_changed(FlAtspiApplication* self,
                                                 FlAtspiAccessible* source) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  emit(self, fl_atspi_dbus_event_object_emit_selection_changed,
       "SelectionChanged", source, "", 0, 0, nullptr);
}

void fl_atspi_application_emit_announcement(FlAtspiApplication* self,
                                            FlAtspiAccessible* source,
                                            const gchar* message,
                                            gboolean assertive) {
  g_return_if_fail(FL_IS_ATSPI_APPLICATION(self));
  g_autoptr(GVariant) data = g_variant_ref_sink(g_variant_new_string(message));
  // The politeness, ATSPI_LIVE_POLITE or ATSPI_LIVE_ASSERTIVE.
  emit(self, fl_atspi_dbus_event_object_emit_announcement, "Announcement",
       source, "", assertive ? 2 : 1, 0, data);
}

GVariant* fl_atspi_application_make_reference(FlAtspiApplication* self,
                                              FlAtspiAccessible* accessible) {
  g_return_val_if_fail(FL_IS_ATSPI_APPLICATION(self), nullptr);
  g_return_val_if_fail(self->connection != nullptr, nullptr);
  return make_reference(self, accessible);
}
