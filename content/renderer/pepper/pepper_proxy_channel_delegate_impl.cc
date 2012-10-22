// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_proxy_channel_delegate_impl.h"

#include "content/common/child_process.h"
#include "content/public/common/sandbox_init.h"

namespace content {

PepperProxyChannelDelegateImpl::~PepperProxyChannelDelegateImpl() {
}

base::MessageLoopProxy* PepperProxyChannelDelegateImpl::GetIPCMessageLoop() {
  // This is called only in the renderer so we know we have a child process.
  DCHECK(ChildProcess::current()) << "Must be in the renderer.";
  return ChildProcess::current()->io_message_loop_proxy();
}

base::WaitableEvent* PepperProxyChannelDelegateImpl::GetShutdownEvent() {
  DCHECK(ChildProcess::current()) << "Must be in the renderer.";
  return ChildProcess::current()->GetShutDownEvent();
}

IPC::PlatformFileForTransit
PepperProxyChannelDelegateImpl::ShareHandleWithRemote(
    base::PlatformFile handle,
    const IPC::SyncChannel& channel,
    bool should_close_source) {
  return BrokerGetFileHandleForProcess(handle, channel.peer_pid(),
                                       should_close_source);
}

}  // namespace content
