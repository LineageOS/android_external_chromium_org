 // Copyright 2013 The Chromium Authors. All rights reserved.
 // Use of this source code is governed by a BSD-style license that can be
 // found in the LICENSE file.

 #ifndef CONTENT_BROWSER_RENDERER_HOST_DEVICE_MOTION_BROWSER_MESSAGE_FILTER_H_
 #define CONTENT_BROWSER_RENDERER_HOST_DEVICE_MOTION_BROWSER_MESSAGE_FILTER_H_

 #include "base/compiler_specific.h"
 #include "base/memory/shared_memory.h"
 #include "content/public/browser/browser_message_filter.h"

 namespace content {

 class DeviceMotionService;
 class RenderProcessHost;

 class DeviceMotionBrowserMessageFilter : public BrowserMessageFilter {
  public:
   DeviceMotionBrowserMessageFilter();

   // BrowserMessageFilter implementation.
   virtual bool OnMessageReceived(const IPC::Message& message,
                                  bool* message_was_ok) OVERRIDE;

  private:
   virtual ~DeviceMotionBrowserMessageFilter();

   void OnDeviceMotionStartPolling();
   void OnDeviceMotionStopPolling();
   void DidStartDeviceMotionPolling();

   bool is_started_;

   DISALLOW_COPY_AND_ASSIGN(DeviceMotionBrowserMessageFilter);
 };

 }  // namespace content

 #endif  // CONTENT_BROWSER_RENDERER_HOST_DEVICE_MOTION_BROWSER_MESSAGE_FILTER_H_
