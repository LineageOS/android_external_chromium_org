// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From dev/ppb_network_proxy_dev.idl modified Thu Jun  6 12:08:31 2013.

#include "ppapi/c/dev/ppb_network_proxy_dev.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/ppb_network_proxy_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

int32_t GetProxyForURL(PP_Instance instance,
                       struct PP_Var url,
                       struct PP_Var* proxy_string,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_NetworkProxy_Dev::GetProxyForURL()";
  EnterInstanceAPI<PPB_NetworkProxy_API> enter(instance, callback);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.functions()->GetProxyForURL(instance,
                                                           url,
                                                           proxy_string,
                                                           enter.callback()));
}

const PPB_NetworkProxy_Dev_0_1 g_ppb_networkproxy_dev_thunk_0_1 = {
  &GetProxyForURL
};

}  // namespace

const PPB_NetworkProxy_Dev_0_1* GetPPB_NetworkProxy_Dev_0_1_Thunk() {
  return &g_ppb_networkproxy_dev_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
