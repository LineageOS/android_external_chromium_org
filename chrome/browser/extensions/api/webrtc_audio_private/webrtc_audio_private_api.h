// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_WEBRTC_AUDIO_PRIVATE_WEBRTC_AUDIO_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_WEBRTC_AUDIO_PRIVATE_WEBRTC_AUDIO_PRIVATE_API_H_

#include "base/memory/ref_counted.h"
#include "base/system_monitor/system_monitor.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/common/extensions/api/webrtc_audio_private.h"
#include "content/public/browser/render_view_host.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "media/audio/audio_device_name.h"
#include "url/gurl.h"

namespace content {
class ResourceContext;
}

namespace extensions {

// Listens for device changes and forwards as an extension event.
class WebrtcAudioPrivateEventService
    : public BrowserContextKeyedAPI,
      public base::SystemMonitor::DevicesChangedObserver {
 public:
  explicit WebrtcAudioPrivateEventService(content::BrowserContext* context);
  virtual ~WebrtcAudioPrivateEventService();

  // BrowserContextKeyedAPI implementation.
  virtual void Shutdown() OVERRIDE;
  static BrowserContextKeyedAPIFactory<WebrtcAudioPrivateEventService>*
      GetFactoryInstance();
  static const char* service_name();

  // base::SystemMonitor::DevicesChangedObserver implementation.
  virtual void OnDevicesChanged(
      base::SystemMonitor::DeviceType device_type) OVERRIDE;

 private:
  friend class BrowserContextKeyedAPIFactory<WebrtcAudioPrivateEventService>;

  void SignalEvent();

  content::BrowserContext* browser_context_;
};

// Common base for WebrtcAudioPrivate functions, that provides a
// couple of optionally-used common implementations.
class WebrtcAudioPrivateFunction : public ChromeAsyncExtensionFunction {
 protected:
  WebrtcAudioPrivateFunction();
  virtual ~WebrtcAudioPrivateFunction();

 protected:
  // Retrieves the list of output device names on the appropriate
  // thread. Call from UI thread, callback will occur on IO thread.
  void GetOutputDeviceNames();

  // Must override this if you call GetOutputDeviceNames. Called on IO thread.
  virtual void OnOutputDeviceNames(
      scoped_ptr<media::AudioDeviceNames> device_names);

  // Retrieve the list of AudioOutputController objects. Calls back
  // via OnControllerList.
  //
  // Returns false on error, in which case it has set |error_| and the
  // entire function should fail.
  //
  // Call from any thread. Callback will occur on originating thread.
  bool GetControllerList(int tab_id);

  // Must override this if you call GetControllerList.
  virtual void OnControllerList(
      const content::RenderViewHost::AudioOutputControllerList& list);

  // Calculates a single HMAC. Call from any thread. Calls back via
  // OnHMACCalculated on UI thread.
  //
  // This function, and device ID HMACs in this API in general use the
  // calling extension's ID as the security origin. The only exception
  // to this rule is when calculating the input device ID HMAC in
  // getAssociatedSink, where we use the provided |securityOrigin|.
  void CalculateHMAC(const std::string& raw_id);

  // Must override this if you call CalculateHMAC.
  virtual void OnHMACCalculated(const std::string& hmac);

  // Calculates a single HMAC, using the extension ID as the security origin.
  //
  // Call only on IO thread.
  std::string CalculateHMACImpl(const std::string& raw_id);

  // Initializes |resource_context_|. Must be called on the UI thread,
  // before any calls to |resource_context()|.
  void InitResourceContext();

  // Callable from any thread. Must previously have called
  // |InitResourceContext()|.
  content::ResourceContext* resource_context() const;

 private:
  content::ResourceContext* resource_context_;

  DISALLOW_COPY_AND_ASSIGN(WebrtcAudioPrivateFunction);
};

class WebrtcAudioPrivateGetSinksFunction : public WebrtcAudioPrivateFunction {
 protected:
  virtual ~WebrtcAudioPrivateGetSinksFunction() {}

 private:
  DECLARE_EXTENSION_FUNCTION("webrtcAudioPrivate.getSinks",
                             WEBRTC_AUDIO_PRIVATE_GET_SINKS);

  // Sequence of events is that we query the list of sinks on the
  // AudioManager's thread, then calculate HMACs on the IO thread,
  // then finish on the UI thread.
  virtual bool RunImpl() OVERRIDE;
  void DoQuery();
  virtual void OnOutputDeviceNames(
      scoped_ptr<media::AudioDeviceNames> raw_ids) OVERRIDE;
  void DoneOnUIThread();
};

class WebrtcAudioPrivateGetActiveSinkFunction
    : public WebrtcAudioPrivateFunction {
 protected:
  virtual ~WebrtcAudioPrivateGetActiveSinkFunction() {}

 private:
  DECLARE_EXTENSION_FUNCTION("webrtcAudioPrivate.getActiveSink",
                             WEBRTC_AUDIO_PRIVATE_GET_ACTIVE_SINK);

  virtual bool RunImpl() OVERRIDE;
  virtual void OnControllerList(
      const content::RenderViewHost::AudioOutputControllerList&
      controllers) OVERRIDE;
  virtual void OnHMACCalculated(const std::string& hmac) OVERRIDE;
};

class WebrtcAudioPrivateSetActiveSinkFunction
    : public WebrtcAudioPrivateFunction {
 public:
  WebrtcAudioPrivateSetActiveSinkFunction();

 protected:
  virtual ~WebrtcAudioPrivateSetActiveSinkFunction();

 private:
  DECLARE_EXTENSION_FUNCTION("webrtcAudioPrivate.setActiveSink",
                             WEBRTC_AUDIO_PRIVATE_SET_ACTIVE_SINK);

  virtual bool RunImpl() OVERRIDE;
  virtual void OnControllerList(
      const content::RenderViewHost::AudioOutputControllerList&
      controllers) OVERRIDE;
  virtual void OnOutputDeviceNames(
      scoped_ptr<media::AudioDeviceNames> device_names) OVERRIDE;
  void SwitchDone();
  void DoneOnUIThread();

  int tab_id_;
  std::string sink_id_;

  // Filled in by OnControllerList.
  content::RenderViewHost::AudioOutputControllerList controllers_;

  // Number of sink IDs we are still waiting for. Can become greater
  // than 0 in OnControllerList, decreases on every OnSinkId call.
  size_t num_remaining_sink_ids_;
};

class WebrtcAudioPrivateGetAssociatedSinkFunction
    : public WebrtcAudioPrivateFunction {
 public:
  WebrtcAudioPrivateGetAssociatedSinkFunction();

 protected:
  virtual ~WebrtcAudioPrivateGetAssociatedSinkFunction();

 private:
  DECLARE_EXTENSION_FUNCTION("webrtcAudioPrivate.getAssociatedSink",
                             WEBRTC_AUDIO_PRIVATE_GET_ASSOCIATED_SINK);

  virtual bool RunImpl() OVERRIDE;

  // This implementation is slightly complicated because of different
  // thread requirements for the various functions we need to invoke.
  //
  // Each worker function will post a task to the appropriate thread
  // for the next one.
  //
  // The sequence of events is:
  // 1. Get the list of source devices on the device thread.
  // 2. Given a source ID for an origin and that security origin, find
  //    the raw source ID. This needs to happen on the IO thread since
  //    we will be using the ResourceContext.
  // 3. Given a raw source ID, get the raw associated sink ID on the
  //    device thread.
  // 4. Given the raw associated sink ID, get its HMAC on the IO thread.
  // 5. Respond with the HMAC of the associated sink ID on the UI thread.

  // Fills in |source_devices_|. Note that these are input devices,
  // not output devices, so don't use
  // |WebrtcAudioPrivateFunction::GetOutputDeviceNames|.
  void GetDevicesOnDeviceThread();

  // Takes the parameters of the function, retrieves the raw source
  // device ID, or the empty string if none.
  void GetRawSourceIDOnIOThread();

  // Gets the raw sink ID for a raw source ID. Sends it to |CalculateHMAC|.
  void GetAssociatedSinkOnDeviceThread(const std::string& raw_source_id);

  // Receives the associated sink ID after its HMAC is calculated.
  virtual void OnHMACCalculated(const std::string& hmac) OVERRIDE;

  // Accessed from UI thread and device thread, but only on one at a
  // time, no locking needed.
  scoped_ptr<api::webrtc_audio_private::GetAssociatedSink::Params> params_;

  // Audio sources (input devices). Filled in by DoWorkOnDeviceThread.
  media::AudioDeviceNames source_devices_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_WEBRTC_AUDIO_PRIVATE_WEBRTC_AUDIO_PRIVATE_API_H_
