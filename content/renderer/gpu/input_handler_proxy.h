// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_INPUT_HANDLER_PROXY_H_
#define CONTENT_RENDERER_GPU_INPUT_HANDLER_PROXY_H_

#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "cc/input/input_handler.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGestureCurve.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGestureCurveTarget.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebActiveWheelFlingParameters.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

namespace content {

class InputHandlerProxyClient;

// This class is a proxy between the content input event filtering and the
// compositor's input handling logic. InputHandlerProxy instances live entirely
// on the compositor thread. Each InputHandler instance handles input events
// intended for a specific WebWidget.
class CONTENT_EXPORT InputHandlerProxy
    : public cc::InputHandlerClient,
      public NON_EXPORTED_BASE(WebKit::WebGestureCurveTarget) {
 public:
  explicit InputHandlerProxy(cc::InputHandler* input_handler);
  virtual ~InputHandlerProxy();

  void SetClient(InputHandlerProxyClient* client);
  void HandleInputEvent(const WebKit::WebInputEvent& event);

  // cc::InputHandlerClient implementation.
  virtual void WillShutdown() OVERRIDE;
  virtual void Animate(base::TimeTicks time) OVERRIDE;
  virtual void MainThreadHasStoppedFlinging() OVERRIDE;

  // WebKit::WebGestureCurveTarget implementation.
  virtual void scrollBy(const WebKit::WebFloatSize& offset);
  virtual void notifyCurrentFlingVelocity(const WebKit::WebFloatSize& velocity);

  bool gesture_scroll_on_impl_thread_for_testing() const {
    return gesture_scroll_on_impl_thread_;
  }

 private:
  enum EventDisposition {
    DidHandle,
    DidNotHandle,
    DropEvent
  };
  // This function processes the input event and determines the disposition, but
  // does not make any calls out to the InputHandlerProxyClient. Some input
  // types defer to helpers.
  EventDisposition HandleInputEventInternal(const WebKit::WebInputEvent& event);

  EventDisposition HandleGestureFling(const WebKit::WebGestureEvent& event);

  // Returns true if we scrolled by the increment.
  bool TouchpadFlingScroll(const WebKit::WebFloatSize& increment);

  // Returns true if we actually had an active fling to cancel.
  bool CancelCurrentFling();

  scoped_ptr<WebKit::WebGestureCurve> fling_curve_;
  // Parameters for the active fling animation, stored in case we need to
  // transfer it out later.
  WebKit::WebActiveWheelFlingParameters fling_parameters_;

  InputHandlerProxyClient* client_;
  cc::InputHandler* input_handler_;

#ifndef NDEBUG
  bool expect_scroll_update_end_;
  bool expect_pinch_update_end_;
#endif
  bool gesture_scroll_on_impl_thread_;
  bool gesture_pinch_on_impl_thread_;
  // This is always false when there are no flings on the main thread, but
  // conservative in the sense that we might not be actually flinging when it is
  // true.
  bool fling_may_be_active_on_main_thread_;

  DISALLOW_COPY_AND_ASSIGN(InputHandlerProxy);
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_INPUT_HANDLER_PROXY_H_
