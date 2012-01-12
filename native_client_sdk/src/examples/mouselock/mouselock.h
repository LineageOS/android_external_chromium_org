// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "ppapi/c/ppb_fullscreen.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/fullscreen.h"
#include "ppapi/cpp/mouse_lock.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/var.h"

namespace mouselock {

class MouseLockInstance : public pp::Instance, public pp::MouseLock {
 public:
  explicit MouseLockInstance(PP_Instance instance)
      : pp::Instance(instance),
        pp::MouseLock(this),
        width_(0),
        height_(0),
        mouse_locked_(false),
        waiting_for_flush_completion_(false),
        callback_factory_(this),
        fullscreen_(this),
        is_context_bound_(false),
        background_scanline_(NULL) {
  }
  virtual ~MouseLockInstance();

  // Called by the browser when the NaCl module is loaded and all ready to go.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);

  // Called by the browser to handle incoming input events.
  virtual bool HandleInputEvent(const pp::InputEvent& event);

  // Called whenever the in-browser window changes size.
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);

  // Called by the browser when mouselock is lost.  This happens when the NaCl
  // module exits fullscreen mode.
  virtual void MouseLockLost();

 private:
  // Return the Cartesian distance between two points.
  double GetDistance(int point_1_x, int point_1_y,
                     int point_2_x, int point_2_y) {
    return sqrt(pow(static_cast<double>(point_1_x - point_2_x), 2) +
                pow(static_cast<double>(point_1_y - point_2_y), 2));
  }

  // Called when mouse lock has been acquired.  Used as a callback to
  // pp::MouseLock.LockMouse().
  void DidLockMouse(int32_t result);

  // Called when the 2D context has been flushed to the browser window.  Used
  // as a callback to pp::Graphics2D.Flush().
  void DidFlush(int32_t result);

  // Creates a new paint buffer, paints it then flush it to the 2D context.  If
  // a flush is pending, this does nothing.
  void Paint();

  // Create a new pp::ImageData and paint the graphics that represent the mouse
  // movement in it.  Return the new pp::ImageData.
  pp::ImageData PaintImage(int width, int height);

  // Fill the image with the backgroud color.
  void ClearToBackground(pp::ImageData* image);

  // Draw a spot in |spot_color| in the center of the image.  The radius of the
  // spot is defined by a constant value in mouselock.cc
  void DrawCenterSpot(pp::ImageData* image, uint32_t spot_color);

  // Draw the needle when the mouse is outside of the central spot.
  void DrawNeedle(pp::ImageData* image, uint32_t needle_color);

  // Print the printf-style format to the "console" via PostMessage.
  void Log(const char* format, ...);

  int width_;
  int height_;

  bool mouse_locked_;
  pp::Point mouse_movement_;
  bool waiting_for_flush_completion_;
  pp::CompletionCallbackFactory<MouseLockInstance> callback_factory_;

  pp::Fullscreen fullscreen_;
  pp::Graphics2D device_context_;
  bool is_context_bound_;
  uint32_t* background_scanline_;
};

}  // namespace mouselock
