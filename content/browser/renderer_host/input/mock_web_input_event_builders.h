// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_MOCK_WEB_INPUT_EVENT_BUILDERS_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_MOCK_WEB_INPUT_EVENT_BUILDERS_H_

#include "base/time/time.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

// Provides sensible creation of default WebInputEvents for testing purposes.

namespace content {

class MockWebMouseEventBuilder {
 public:
  static WebKit::WebMouseEvent Build(WebKit::WebInputEvent::Type type);
  static WebKit::WebMouseEvent Build(WebKit::WebInputEvent::Type type,
                                     int window_x,
                                     int window_y,
                                     int modifiers);
};

class MockWebMouseWheelEventBuilder {
 public:
  static WebKit::WebMouseWheelEvent Build(
      WebKit::WebMouseWheelEvent::Phase phase);
  static WebKit::WebMouseWheelEvent Build(float dx,
                                          float dy,
                                          int modifiers,
                                          bool precise);
};

class MockWebKeyboardEventBuilder {
 public:
  static NativeWebKeyboardEvent Build(WebKit::WebInputEvent::Type type);
};

class MockWebGestureEventBuilder {
 public:
  static WebKit::WebGestureEvent Build(
      WebKit::WebInputEvent::Type type,
      WebKit::WebGestureEvent::SourceDevice sourceDevice);
  static WebKit::WebGestureEvent BuildScrollUpdate(float dx,
                                                   float dY,
                                                   int modifiers);
  static WebKit::WebGestureEvent BuildPinchUpdate(float scale,
                                                  float anchor_x,
                                                  float anchor_y,
                                                  int modifiers);
  static WebKit::WebGestureEvent BuildFling(
      float velocity_x,
      float velocity_y,
      WebKit::WebGestureEvent::SourceDevice source_device);
};

struct MockWebTouchEvent : public WebKit::WebTouchEvent {
 public:
  MockWebTouchEvent();

  // Mark all the points as stationary, and remove any released points.
  void ResetPoints();

  // Adds an additional point to the touch list, returning the point's index.
  int PressPoint(int x, int y);
  void MovePoint(int index, int x, int y);
  void ReleasePoint(int index);

  void SetTimestamp(base::TimeDelta timestamp);
};

class MockWebTouchEventBuilder {
 public:
  static MockWebTouchEvent Build(WebKit::WebInputEvent::Type type);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_MOCK_WEB_INPUT_EVENT_BUILDERS_H_
