// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_DARWIN_MACOS_FRAMEWORK_SOURCE_FLUTTERWINDOWCONTROLLER_H_
#define FLUTTER_SHELL_PLATFORM_DARWIN_MACOS_FRAMEWORK_SOURCE_FLUTTERWINDOWCONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "flutter/shell/platform/darwin/common/framework/Headers/FlutterMacros.h"
#import "flutter/shell/platform/darwin/macos/framework/Headers/FlutterViewController.h"

@class FlutterEngine;

@interface FlutterWindowController : NSObject

@property(nonatomic, weak) FlutterEngine* engine;

- (void)closeAllWindows;

- (void)fixMoveRunLoopModeIfNeeded;

@end

struct FlutterWindowRect {
  double left;
  double top;
  double width;
  double height;

  static FlutterWindowRect fromNSRect(const NSRect& rect) {
    return {
        rect.origin.x,
        rect.origin.y,
        rect.size.width,
        rect.size.height,
    };
  }

  NSRect toNSRect() const { return NSMakeRect(left, top, width, height); }
};

struct FlutterWindowSize {
  double width;
  double height;

  static FlutterWindowSize fromNSSize(const NSSize& size) {
    return {
        size.width,
        size.height,
    };
  }
};

struct FlutterWindowOffset {
  double x;
  double y;

  static FlutterWindowOffset fromNSPoint(const NSPoint& point) {
    return {
        point.x,
        point.y,
    };
  }
};

struct FlutterWindowConstraints {
  double min_width;
  double min_height;
  double max_width;
  double max_height;
};

struct FlutterWindowCreationRequest {
  bool has_size;
  struct FlutterWindowSize size;
  bool has_constraints;
  struct FlutterWindowConstraints constraints;
  bool resizable;
  int64_t parent_view_id;
  void (*on_should_close)();
  void (*on_will_close)();
  void (*notify_listeners)();
  // For sized to content windows with positioner returns the desired window position for given
  // configuration. All coordinates are in logical space.
  FlutterWindowRect* (*on_get_window_position)(const FlutterWindowSize& child_size,
                                               const FlutterWindowRect& parent_rect,
                                               const FlutterWindowRect& output_rect);
};

extern "C" {

// NOLINTBEGIN(google-objc-function-naming)

FLUTTER_DARWIN_EXPORT
int64_t InternalFlutter_WindowController_CreateRegularWindow(
    int64_t engine_id,
    const FlutterWindowCreationRequest* request);

FLUTTER_DARWIN_EXPORT
int64_t InternalFlutter_WindowController_CreateDialogWindow(
    int64_t engine_id,
    const FlutterWindowCreationRequest* request);

FLUTTER_DARWIN_EXPORT
int64_t InternalFlutter_WindowController_CreateTooltipWindow(
    int64_t engine_id,
    const FlutterWindowCreationRequest* request);

FLUTTER_DARWIN_EXPORT
int64_t InternalFlutter_WindowController_CreatePopupWindow(
    int64_t engine_id,
    const FlutterWindowCreationRequest* request);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_Destroy(int64_t engine_id, void* window);

FLUTTER_DARWIN_EXPORT
void* InternalFlutter_Window_GetHandle(int64_t engine_id, FlutterViewIdentifier view_id);

FLUTTER_DARWIN_EXPORT
FlutterWindowSize InternalFlutter_Window_GetContentSize(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetContentSize(void* window, const FlutterWindowSize* size);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetConstraints(void* window,
                                           const FlutterWindowConstraints* constraints);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetTitle(void* window, const char* title);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetMaximized(void* window, bool maximized);

FLUTTER_DARWIN_EXPORT
bool InternalFlutter_Window_IsMaximized(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_Minimize(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_Unminimize(void* window);

FLUTTER_DARWIN_EXPORT
bool InternalFlutter_Window_IsMinimized(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetFullScreen(void* window, bool fullScreen);

FLUTTER_DARWIN_EXPORT
bool InternalFlutter_Window_IsFullScreen(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_Activate(void* window);

FLUTTER_DARWIN_EXPORT
char* InternalFlutter_Window_GetTitle(void* window);

FLUTTER_DARWIN_EXPORT
bool InternalFlutter_Window_IsActivated(void* window);

FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_UpdatePosition(void* window);

FLUTTER_DARWIN_EXPORT
FlutterWindowOffset InternalFlutter_Window_GetOffsetInParent(void* window);

// Sets whether the window is drawn with the system title bar. An undecorated
// window has its content extended over the whole window, so the app draws its
// own title bar. Does nothing to a window that never had a title bar.
//
// The window buttons are left in place, see
// |InternalFlutter_Window_SetWindowButtonsVisible|.
FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetDecorated(void* window, bool decorated);

// Returns whether the window is drawn with the system title bar. A window that
// never had a title bar is not decorated.
FLUTTER_DARWIN_EXPORT
bool InternalFlutter_Window_IsDecorated(void* window);

// Sets whether the close, minimize and zoom buttons are shown.
FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_SetWindowButtonsVisible(void* window, bool visible);

// Returns the area the close, minimize and zoom buttons occupy, in logical
// coordinates relative to the top left of the window's content. The area is
// empty if the buttons are hidden.
FLUTTER_DARWIN_EXPORT
FlutterWindowRect InternalFlutter_Window_GetWindowButtonsRect(void* window);

// Starts an interactive move of the window using the event currently being
// handled. Does nothing if that is not an event for this window that a drag can
// be started from, i.e. a mouse button being pressed or dragged.
FLUTTER_DARWIN_EXPORT
void InternalFlutter_Window_BeginMoveDrag(void* window);

// NOLINTEND(google-objc-function-naming)
}

#endif  // FLUTTER_SHELL_PLATFORM_DARWIN_MACOS_FRAMEWORK_SOURCE_FLUTTERWINDOWCONTROLLER_H_
