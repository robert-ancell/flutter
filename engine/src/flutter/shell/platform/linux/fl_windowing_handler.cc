// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_windowing_handler.h"

#include "flutter/shell/platform/linux/public/flutter_linux/fl_view.h"

typedef struct {
  GWeakRef engine;
} FlWindowingHandlerPrivate;

enum { SIGNAL_CREATE_WINDOW, LAST_SIGNAL };

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(FlWindowingHandler,
                           fl_windowing_handler,
                           G_TYPE_OBJECT)

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

static void fl_windowing_handler_init(FlWindowingHandler* self) {}

G_MODULE_EXPORT FlWindowingHandler* fl_windowing_handler_new(FlEngine* engine) {
  g_return_val_if_fail(FL_IS_ENGINE(engine), nullptr);

  FlWindowingHandler* self = FL_WINDOWING_HANDLER(
      g_object_new(fl_windowing_handler_get_type(), nullptr));
  FlWindowingHandlerPrivate* priv =
      reinterpret_cast<FlWindowingHandlerPrivate*>(
          fl_windowing_handler_get_instance_private(self));

  g_weak_ref_init(&priv->engine, engine);

  return self;
}
