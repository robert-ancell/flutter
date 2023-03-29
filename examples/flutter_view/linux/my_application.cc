// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "my_application.h"

#include <flutter_linux/flutter_linux.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"

struct _MyApplication {
  GtkApplication parent_instance;

  // Channel to receive platform view requests from Flutter.
  FlBasicMessageChannel* increment_channel;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

// Handle messages from Flutter.
static void increment_message_cb(FlBasicMessageChannel* channel,
                                        FlValue* message,
					 FlBasicMessageChannelResponseHandle* response_handle,
                                        gpointer user_data) {
  //MyApplication* self = MY_APPLICATION(user_data);

   g_autoptr(FlValue) response = fl_value_new_string("");
   fl_basic_message_channel_respond(channel, response_handle, response, nullptr);
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object) {
  MyApplication* self = MY_APPLICATION(object);

  g_clear_object(&self->increment_channel);

  G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  GtkWindow* window =
      GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));

  gtk_window_set_default_size(window, 1280, 720);
  gtk_widget_show(GTK_WIDGET(window));

  g_autoptr(FlDartProject) project = fl_dart_project_new();
  FlView* view = fl_view_new(project);
  gtk_widget_show(GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  // Create channel to Flutter.
  FlEngine* engine = fl_view_get_engine(view);
  g_autoptr(FlStringCodec) codec = fl_string_codec_new();
  self->increment_channel =
      fl_basic_message_channel_new(fl_engine_get_binary_messenger(engine), "increment",
                            FL_MESSAGE_CODEC(codec));
  fl_basic_message_channel_set_message_handler(
      self->increment_channel, increment_message_cb, self, nullptr);

  fl_register_plugins(FL_PLUGIN_REGISTRY(view));

  gtk_widget_grab_focus(GTK_WIDGET(view));
}

static void my_application_class_init(MyApplicationClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
  G_APPLICATION_CLASS(klass)->activate = my_application_activate;
}

static void my_application_init(MyApplication* self) {}

MyApplication* my_application_new() {
  return MY_APPLICATION(g_object_new(my_application_get_type(),
                                     "application-id", APPLICATION_ID, "flags",
                                     G_APPLICATION_NON_UNIQUE, nullptr));
}
