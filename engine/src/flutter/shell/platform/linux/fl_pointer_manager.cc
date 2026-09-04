// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_pointer_manager.h"

#include "flutter/shell/platform/linux/fl_engine_private.h"

static constexpr int kMicrosecondsPerMillisecond = 1000;

// State of a single pointing device. Each device is reported to Flutter as a
// separate pointer, so their buttons and locations are tracked separately.
typedef struct {
  // ID Flutter uses to refer to this device.
  int32_t id;

  // The kind of device this is.
  FlutterPointerDeviceKind kind;

  // TRUE if this pointer is inside the view, used for generating missing add
  // events.
  gboolean pointer_inside;

  // Pointer button state recorded for sending status updates.
  int64_t button_state;

  // TRUE if a leave event was received while a button was pressed, and the
  // matching remove event has not been sent yet.
  gboolean leave_pending;

  // Last known pointer position and state, used when synthesizing events.
  gdouble last_x;
  gdouble last_y;
  gdouble last_rotation;
  gdouble last_pressure;
} FlPointerDevice;

struct _FlPointerManager {
  GObject parent_instance;

  // Engine to send pointer events to.
  GWeakRef engine;

  // ID to mark events with.
  FlutterViewId view_id;

  // Table of #GdkDevice to #FlPointerDevice for the devices that have
  // generated events.
  GHashTable* devices;

  // ID to give the next device that generates an event.
  int32_t next_device_id;
};

G_DEFINE_TYPE(FlPointerManager, fl_pointer_manager, G_TYPE_OBJECT);

// 8 corresponds to mouse back button on both x11 and wayland
static constexpr guint kMouseButtonBack = 8;

// 9 corresponds to mouse forward button on both x11 and wayland
static constexpr guint kMouseButtonForward = 9;

// Convert a GDK button ID into a Flutter button ID
static gboolean get_mouse_button(guint gdk_button, int64_t* button) {
  switch (gdk_button) {
    case GDK_BUTTON_PRIMARY:
      *button = kFlutterPointerButtonMousePrimary;
      return TRUE;
    case GDK_BUTTON_MIDDLE:
      *button = kFlutterPointerButtonMouseMiddle;
      return TRUE;
    case GDK_BUTTON_SECONDARY:
      *button = kFlutterPointerButtonMouseSecondary;
      return TRUE;
    case kMouseButtonBack:
      *button = kFlutterPointerButtonMouseBack;
      return TRUE;
    case kMouseButtonForward:
      *button = kFlutterPointerButtonMouseForward;
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean get_button(FlutterPointerDeviceKind device_kind,
                           guint gdk_button,
                           int64_t* button) {
  if (device_kind == kFlutterPointerDeviceKindStylus ||
      device_kind == kFlutterPointerDeviceKindInvertedStylus) {
    // GDK button names describe the physical button action, where "primary"
    // is the stylus tip contact. Flutter stylus button names reserve "primary"
    // for the first barrel button, so the GDK secondary button maps there.
    switch (gdk_button) {
      case GDK_BUTTON_PRIMARY:
        *button = kFlutterPointerButtonStylusContact;
        return TRUE;
      case GDK_BUTTON_SECONDARY:
        *button = kFlutterPointerButtonStylusPrimary;
        return TRUE;
      case GDK_BUTTON_MIDDLE:
        *button = kFlutterPointerButtonStylusSecondary;
        return TRUE;
      default:
        return FALSE;
    }
  }

  return get_mouse_button(gdk_button, button);
}

// Releases the reference held on a device used as a key in the device table.
static void unref_device(gpointer device) {
  // Events that don't report a device are tracked with a NULL key.
  if (device == nullptr) {
    return;
  }
  g_object_unref(device);
}

// Gets the kind of pointer a GDK device is.
static FlutterPointerDeviceKind get_device_kind(GdkDevice* device) {
  if (device == nullptr) {
    return kFlutterPointerDeviceKindMouse;
  }

  switch (gdk_device_get_source(device)) {
    case GDK_SOURCE_PEN:
    case GDK_SOURCE_CURSOR:
    case GDK_SOURCE_TABLET_PAD:
      return kFlutterPointerDeviceKindStylus;
    case GDK_SOURCE_ERASER:
      return kFlutterPointerDeviceKindInvertedStylus;
    case GDK_SOURCE_TOUCHSCREEN:
      return kFlutterPointerDeviceKindTouch;
    case GDK_SOURCE_TOUCHPAD:  // trackpad device type is reserved for gestures
    case GDK_SOURCE_TRACKPOINT:
    case GDK_SOURCE_KEYBOARD:
    case GDK_SOURCE_MOUSE:
      return kFlutterPointerDeviceKindMouse;
  }
}

// Gets the state being tracked for a device, creating it if this device
// hasn't generated an event before.
static FlPointerDevice* get_device(FlPointerManager* self, GdkDevice* device) {
  FlPointerDevice* state =
      static_cast<FlPointerDevice*>(g_hash_table_lookup(self->devices, device));
  if (state != nullptr) {
    return state;
  }

  state = g_new0(FlPointerDevice, 1);
  state->id = self->next_device_id;
  // Skip over the ID reserved for pan and zoom events.
  self->next_device_id = self->next_device_id == kMousePointerDeviceId
                             ? kPointerPanZoomDeviceId + 1
                             : self->next_device_id + 1;
  state->kind = get_device_kind(device);
  // The device is referenced so it stays alive while it is a key in this
  // table, e.g. if it is unplugged.
  g_hash_table_insert(
      self->devices, device != nullptr ? g_object_ref(device) : nullptr, state);

  return state;
}

// Records the most recent pointer state so that events can be synthesized
// from it later.
static void record_pointer_state(FlPointerDevice* device,
                                 gdouble x,
                                 gdouble y,
                                 gdouble rotation,
                                 gdouble pressure) {
  device->last_x = x;
  device->last_y = y;
  device->last_rotation = rotation;
  device->last_pressure = pressure;
}

// Cancels the buttons pressed on a device, e.g. because their releases will
// never be received.
static gboolean cancel_device(FlPointerManager* self,
                              FlPointerDevice* device,
                              guint event_time) {
  // Nothing to do if no buttons are pressed.
  if (device->button_state == 0) {
    return FALSE;
  }

  device->button_state = 0;

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  fl_engine_send_mouse_pointer_event(
      engine, self->view_id, kCancel, event_time * kMicrosecondsPerMillisecond,
      device->last_x, device->last_y, device->kind, device->id, 0, 0,
      device->button_state, device->last_rotation, device->last_pressure);

  // The pointer left the view while the button was down, so the remove event
  // was delayed until the button was released. That release will never
  // arrive, so remove the pointer now.
  if (device->leave_pending) {
    fl_engine_send_mouse_pointer_event(
        engine, self->view_id, kRemove,
        event_time * kMicrosecondsPerMillisecond, device->last_x,
        device->last_y, device->kind, device->id, 0, 0, device->button_state,
        device->last_rotation, device->last_pressure);
    device->pointer_inside = FALSE;
    device->leave_pending = FALSE;
  }

  return TRUE;
}

// Generates a mouse pointer event if the pointer appears inside the window.
static void ensure_pointer_added(FlPointerManager* self,
                                 FlPointerDevice* device,
                                 guint event_time,
                                 gdouble x,
                                 gdouble y,
                                 gdouble rotation,
                                 gdouble pressure) {
  record_pointer_state(device, x, y, rotation, pressure);

  // The pointer is generating events again, so it is inside the view.
  device->leave_pending = FALSE;

  if (device->pointer_inside) {
    return;
  }
  device->pointer_inside = TRUE;

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return;
  }

  fl_engine_send_mouse_pointer_event(engine, self->view_id, kAdd,
                                     event_time * kMicrosecondsPerMillisecond,
                                     x, y, device->kind, device->id, 0, 0,
                                     device->button_state, rotation, pressure);
}

static void fl_pointer_manager_dispose(GObject* object) {
  FlPointerManager* self = FL_POINTER_MANAGER(object);

  g_weak_ref_clear(&self->engine);

  g_clear_pointer(&self->devices, g_hash_table_unref);

  G_OBJECT_CLASS(fl_pointer_manager_parent_class)->dispose(object);
}

static void fl_pointer_manager_class_init(FlPointerManagerClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_pointer_manager_dispose;
}

static void fl_pointer_manager_init(FlPointerManager* self) {
  self->next_device_id = kMousePointerDeviceId;
  self->devices = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                        unref_device, g_free);
}

FlPointerManager* fl_pointer_manager_new(FlutterViewId view_id,
                                         FlEngine* engine) {
  FlPointerManager* self =
      FL_POINTER_MANAGER(g_object_new(fl_pointer_manager_get_type(), nullptr));

  self->view_id = view_id;
  g_weak_ref_init(&self->engine, engine);

  return self;
}

gboolean fl_pointer_manager_handle_button_press(FlPointerManager* self,
                                                guint event_time,
                                                GdkDevice* device,
                                                gdouble x,
                                                gdouble y,
                                                guint gdk_button,
                                                gdouble rotation,
                                                gdouble pressure) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  FlPointerDevice* pointer = get_device(self, device);

  int64_t button;
  if (!get_button(pointer->kind, gdk_button, &button)) {
    return FALSE;
  }

  ensure_pointer_added(self, pointer, event_time, x, y, rotation, pressure);

  // GDK never sends two presses of the same button without a release in
  // between, so if Flutter thinks this button is already down then the release
  // was lost, e.g. it was delivered to the window manager because it ended an
  // interactive move or resize. Cancel the stale press so this one is not
  // dropped.
  if ((pointer->button_state & button) != 0) {
    cancel_device(self, pointer, event_time);
  }

  int old_button_state = pointer->button_state;
  FlutterPointerPhase phase = kMove;
  pointer->button_state ^= button;
  phase = old_button_state == 0 ? kDown : kMove;

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  fl_engine_send_mouse_pointer_event(engine, self->view_id, phase,
                                     event_time * kMicrosecondsPerMillisecond,
                                     x, y, pointer->kind, pointer->id, 0, 0,
                                     pointer->button_state, rotation, pressure);

  return TRUE;
}

gboolean fl_pointer_manager_handle_button_release(FlPointerManager* self,
                                                  guint event_time,
                                                  GdkDevice* device,
                                                  gdouble x,
                                                  gdouble y,
                                                  guint gdk_button,
                                                  gdouble rotation,
                                                  gdouble pressure) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  FlPointerDevice* pointer = get_device(self, device);

  int64_t button;
  if (!get_button(pointer->kind, gdk_button, &button)) {
    return FALSE;
  }

  record_pointer_state(pointer, x, y, rotation, pressure);

  // Drop the event if Flutter already thinks the button is up.
  if ((pointer->button_state & button) == 0) {
    return FALSE;
  }

  FlutterPointerPhase phase = kMove;
  pointer->button_state ^= button;

  phase = pointer->button_state == 0 ? kUp : kMove;

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  fl_engine_send_mouse_pointer_event(engine, self->view_id, phase,
                                     event_time * kMicrosecondsPerMillisecond,
                                     x, y, pointer->kind, pointer->id, 0, 0,
                                     pointer->button_state, rotation, pressure);

  return TRUE;
}

gboolean fl_pointer_manager_handle_motion(FlPointerManager* self,
                                          guint event_time,
                                          GdkDevice* device,
                                          gdouble x,
                                          gdouble y,
                                          gdouble rotation,
                                          gdouble pressure) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  FlPointerDevice* pointer = get_device(self, device);

  ensure_pointer_added(self, pointer, event_time, x, y, rotation, pressure);

  fl_engine_send_mouse_pointer_event(
      engine, self->view_id, pointer->button_state != 0 ? kMove : kHover,
      event_time * kMicrosecondsPerMillisecond, x, y, pointer->kind,
      pointer->id, 0, 0, pointer->button_state, rotation, pressure);

  return TRUE;
}

gboolean fl_pointer_manager_handle_enter(FlPointerManager* self,
                                         guint event_time,
                                         GdkDevice* device,
                                         gdouble x,
                                         gdouble y,
                                         gdouble rotation,
                                         gdouble pressure) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  ensure_pointer_added(self, get_device(self, device), event_time, x, y,
                       rotation, pressure);

  return TRUE;
}

gboolean fl_pointer_manager_handle_leave(FlPointerManager* self,
                                         guint event_time,
                                         GdkDevice* device,
                                         gdouble x,
                                         gdouble y,
                                         gdouble rotation,
                                         gdouble pressure) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  g_autoptr(FlEngine) engine = FL_ENGINE(g_weak_ref_get(&self->engine));
  if (engine == nullptr) {
    return FALSE;
  }

  FlPointerDevice* pointer = get_device(self, device);

  if (!pointer->pointer_inside) {
    return TRUE;
  }

  // Don't remove pointer while button is down; In case of dragging outside of
  // window with mouse grab active Gtk will send another leave notify on
  // release. Remember the leave so the pointer can still be removed if that
  // release is never delivered, e.g. because the grab was broken.
  if (pointer->button_state != 0) {
    record_pointer_state(pointer, x, y, rotation, pressure);
    pointer->leave_pending = TRUE;
    return TRUE;
  }

  fl_engine_send_mouse_pointer_event(engine, self->view_id, kRemove,
                                     event_time * kMicrosecondsPerMillisecond,
                                     x, y, pointer->kind, pointer->id, 0, 0,
                                     pointer->button_state, rotation, pressure);
  pointer->pointer_inside = FALSE;
  pointer->leave_pending = FALSE;

  return TRUE;
}

gboolean fl_pointer_manager_handle_grab_broken(FlPointerManager* self,
                                               guint event_time) {
  g_return_val_if_fail(FL_IS_POINTER_MANAGER(self), FALSE);

  // Every device has lost its grab, e.g. the window is being moved by the
  // window manager.
  gboolean handled = FALSE;
  GHashTableIter iter;
  gpointer value;
  g_hash_table_iter_init(&iter, self->devices);
  while (g_hash_table_iter_next(&iter, nullptr, &value)) {
    FlPointerDevice* pointer = static_cast<FlPointerDevice*>(value);
    if (cancel_device(self, pointer, event_time)) {
      handled = TRUE;
    }
  }

  return handled;
}
