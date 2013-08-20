// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_RENDERER_CONNECTION_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_RENDERER_CONNECTION_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_message_filter.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"

class GURL;

namespace ppapi {
namespace proxy {
class ResourceMessageCallParams;
}
}

namespace content {

class BrowserPpapiHostImpl;
struct PepperRendererInstanceData;

// This class represents a connection from the browser to the renderer for
// sending/receiving pepper ResourceHost related messages. When the browser
// and renderer communicate about ResourceHosts, they should pass the plugin
// process ID to identify which plugin they are talking about.
class PepperRendererConnection : public BrowserMessageFilter {
 public:
  explicit PepperRendererConnection(int render_process_id);

  // BrowserMessageFilter overrides.
  virtual bool OnMessageReceived(const IPC::Message& msg,
                                 bool* message_was_ok) OVERRIDE;

 private:
  virtual ~PepperRendererConnection();

  // Returns the host for the child process for the given |child_process_id|.
  // If |child_process_id| is 0, returns the host owned by this
  // PepperRendererConnection, which serves as the host for in-process plugins.
  BrowserPpapiHostImpl* GetHostForChildProcess(int child_process_id) const;

  void OnMsgCreateResourceHostFromHost(
      int routing_id,
      int child_process_id,
      const ppapi::proxy::ResourceMessageCallParams& params,
      PP_Instance instance,
      const IPC::Message& nested_msg);

  void OnMsgFileRefGetInfoForRenderer(
      int routing_id,
      int child_process_id,
      int32_t sequence_num,
      const std::vector<PP_Resource>& resources);

  void OnMsgDidCreateInProcessInstance(
      PP_Instance instance,
      const PepperRendererInstanceData& instance_data);
  void OnMsgDidDeleteInProcessInstance(PP_Instance instance);

  int render_process_id_;

  // We have a single BrowserPpapiHost per-renderer for all in-process plugins
  // running. This is just a work-around allowing new style resources to work
  // with the browser when running in-process but it means that plugin-specific
  // information (like the plugin name) won't be available.
  scoped_ptr<BrowserPpapiHostImpl> in_process_host_;

  DISALLOW_COPY_AND_ASSIGN(PepperRendererConnection);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_RENDERER_CONNECTION_H_
