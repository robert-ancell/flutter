// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_windowing_handler.h"

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/linux/fl_engine_private.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_view.h"

typedef struct {
  GWeakRef engine;

  GHashTable* windows_by_view_id;
} FlWindowingHandlerPrivate;

enum { SIGNAL_CREATE_WINDOW, LAST_SIGNAL };

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(FlWindowingHandler,
                           fl_windowing_handler,
                           G_TYPE_OBJECT)

typedef struct {
  GtkWindow* window;
  FlView* view;
} WindowData;

static WindowData* window_data_new(GtkWindow* window, FlView* view) {
  WindowData* data = g_new0(WindowData, 1);
  data->window = GTK_WINDOW(g_object_ref(window));
  data->view = FL_VIEW(g_object_ref(view));
  return data;
}

static void window_data_free(WindowData* data) {
  g_object_unref(data->window);
  g_object_unref(data->view);
  g_free(data);
}

static WindowData* get_window_data(FlWindowingHandler* self, int64_t view_id) {
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  return static_cast<WindowData*>(
      g_hash_table_lookup(priv->windows_by_view_id, GINT_TO_POINTER(view_id)));
}

static GtkWindow* fl_windowing_handler_create_window(
    FlWindowingHandler* handler,
    FlView* view) {
  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  return GTK_WINDOW(window);
}

static void fl_windowing_handler_dispose(GObject* object) {
  FlWindowingHandler* self = FL_WINDOWING_HANDLER(object);
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  g_weak_ref_clear(&priv->engine);
  g_clear_pointer(&priv->windows_by_view_id, g_hash_table_unref);

  G_OBJECT_CLASS(fl_windowing_handler_parent_class)->dispose(object);
}

static void fl_windowing_handler_class_init(FlWindowingHandlerClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_windowing_handler_dispose;

  klass->create_window = fl_windowing_handler_create_window;

  signals[SIGNAL_CREATE_WINDOW] = g_signal_new(
      "create-window", fl_windowing_handler_get_type(), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(FlWindowingHandlerClass, create_window),
      g_signal_accumulator_first_wins, nullptr, nullptr, GTK_TYPE_WINDOW, 1,
      fl_view_get_type());
}

static void fl_windowing_handler_init(FlWindowingHandler* self) {
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  priv->windows_by_view_id =
      g_hash_table_new_full(g_direct_hash, g_direct_equal, nullptr,
                            reinterpret_cast<GDestroyNotify>(window_data_free));
}

FlWindowingHandler* fl_windowing_handler_new(FlEngine* engine) {
  g_return_val_if_fail(FL_IS_ENGINE(engine), nullptr);

  FlWindowingHandler* self = FL_WINDOWING_HANDLER(
      g_object_new(fl_windowing_handler_get_type(), nullptr));
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  g_weak_ref_init(&priv->engine, engine);

  return self;
}

extern "C" {
G_MODULE_EXPORT bool InternalFlutterLinux_HasTopLevelWindows(
    int64_t engine_id) {
  FlEngine* engine = fl_engine_for_id(engine_id);
  FlWindowingHandler* self = fl_engine_get_windowing_handler(engine);
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  // FIXME: Make sure all windows created through this API.
  return g_hash_table_size(priv->windows_by_view_id) > 0;
}

G_MODULE_EXPORT FlutterViewId InternalFlutterLinux_CreateRegularWindow(
    int64_t engine_id) {
  FlEngine* engine = fl_engine_for_id(engine_id);
  FlWindowingHandler* self = fl_engine_get_windowing_handler(engine);
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  FlView* view = fl_view_new_for_engine(engine);
  gtk_widget_show(GTK_WIDGET(view));

  GtkWindow* window = nullptr;
  g_signal_emit(self, signals[SIGNAL_CREATE_WINDOW], 0, view, &window);
  if (window == nullptr) {
    // FIXME: error
    return 0;
  }

  WindowData* data = window_data_new(GTK_WINDOW(window), view);
  g_hash_table_insert(priv->windows_by_view_id,
                      GINT_TO_POINTER(fl_view_get_id(view)), data);

  return fl_view_get_id(view);
}

G_MODULE_EXPORT GtkWindow* InternalFlutterLinux_GetGtkWindow(
    int64_t engine_id,
    FlutterViewId view_id) {
  FlEngine* engine = fl_engine_for_id(engine_id);
  FlWindowingHandler* self = fl_engine_get_windowing_handler(engine);

  WindowData* data = get_window_data(self, view_id);
  return data != nullptr ? data->window : nullptr;
}
}
