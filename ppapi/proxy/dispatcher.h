// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_DISPATCHER_H_
#define PPAPI_PROXY_DISPATCHER_H_

#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/tracked_objects.h"
#include "ipc/ipc_channel_proxy.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/ppp.h"
#include "ppapi/proxy/proxy_channel.h"
#include "ppapi/proxy/interface_list.h"
#include "ppapi/proxy/interface_proxy.h"
#include "ppapi/proxy/plugin_var_tracker.h"
#include "ppapi/shared_impl/api_id.h"

namespace ppapi {

namespace proxy {

class VarSerializationRules;

// An interface proxy can represent either end of a cross-process interface
// call. The "source" side is where the call is invoked, and the "target" side
// is where the call ends up being executed.
//
// Plugin side                          | Browser side
// -------------------------------------|--------------------------------------
//                                      |
//    "Source"                          |    "Target"
//    InterfaceProxy ----------------------> InterfaceProxy
//                                      |
//                                      |
//    "Target"                          |    "Source"
//    InterfaceProxy <---------------------- InterfaceProxy
//                                      |
class PPAPI_PROXY_EXPORT Dispatcher : public ProxyChannel {
 public:
  virtual ~Dispatcher();

  // Returns true if the dispatcher is on the plugin side, or false if it's the
  // browser side.
  virtual bool IsPlugin() const = 0;

  VarSerializationRules* serialization_rules() const {
    return serialization_rules_.get();
  }

  // Returns a non-owning pointer to the interface proxy for the given ID, or
  // NULL if the ID isn't found. This will create the proxy if it hasn't been
  // created so far.
  InterfaceProxy* GetInterfaceProxy(ApiID id);

  // Returns the pointer to the IO thread for processing IPC messages.
  // TODO(brettw) remove this. It's a hack to support the Flash
  // ModuleLocalThreadAdapter. When the thread stuff is sorted out, this
  // implementation detail should be hidden.
  base::MessageLoopProxy* GetIPCMessageLoop();

  // Adds the given filter to the IO thread. Takes ownership of the pointer.
  void AddIOThreadMessageFilter(IPC::ChannelProxy::MessageFilter* filter);

  // TODO(brettw): What is this comment referring to?
  // Called if the remote side is declaring to us which interfaces it supports
  // so we don't have to query for each one. We'll pre-create proxies for
  // each of the given interfaces.

  // IPC::Listener implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  PP_GetInterface_Func local_get_interface() const {
    return local_get_interface_;
  }

 protected:
  explicit Dispatcher(PP_GetInterface_Func local_get_interface);

  // Setter for the derived classes to set the appropriate var serialization.
  // Takes one reference of the given pointer, which must be on the heap.
  void SetSerializationRules(VarSerializationRules* var_serialization_rules);

  // Called when an invalid message is received from the remote site. The
  // default implementation does nothing, derived classes can override.
  virtual void OnInvalidMessageReceived();

  bool disallow_trusted_interfaces() const {
    return disallow_trusted_interfaces_;
  }

 private:
  friend class HostDispatcherTest;
  friend class PluginDispatcherTest;

  // Lists all lazily-created interface proxies.
  scoped_ptr<InterfaceProxy> proxies_[API_ID_COUNT];

  bool disallow_trusted_interfaces_;

  PP_GetInterface_Func local_get_interface_;

  scoped_refptr<VarSerializationRules> serialization_rules_;

  DISALLOW_COPY_AND_ASSIGN(Dispatcher);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_DISPATCHER_H_
