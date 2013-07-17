// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_shared_memory_message_filter.h"

#include "base/memory/scoped_ptr.h"
#include "base/memory/shared_memory.h"
#include "base/process_util.h"
#include "content/public/common/content_client.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "webkit/plugins/ppapi/host_globals.h"

namespace chrome {

PepperSharedMemoryMessageFilter::PepperSharedMemoryMessageFilter(
    content::RendererPpapiHost* host)
    : InstanceMessageFilter(host->GetPpapiHost()),
      host_(host) {
}

PepperSharedMemoryMessageFilter::~PepperSharedMemoryMessageFilter() {
}

bool PepperSharedMemoryMessageFilter::OnInstanceMessageReceived(
    const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PepperSharedMemoryMessageFilter, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_SharedMemory_CreateSharedMemory,
                        OnHostMsgCreateSharedMemory)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool PepperSharedMemoryMessageFilter::Send(IPC::Message* msg) {
  return host_->GetPpapiHost()->Send(msg);
}

void PepperSharedMemoryMessageFilter::OnHostMsgCreateSharedMemory(
    PP_Instance instance,
    uint32_t size,
    int* host_handle_id,
    ppapi::proxy::SerializedHandle* plugin_handle) {
  plugin_handle->set_null_shmem();
  *host_handle_id = -1;
  scoped_ptr<base::SharedMemory> shm(content::RenderThread::Get()->
      HostAllocateSharedMemoryBuffer(size).Pass());
  if (!shm.get())
    return;

  base::SharedMemoryHandle host_shm_handle;
  shm->ShareToProcess(base::GetCurrentProcessHandle(), &host_shm_handle);
  *host_handle_id = content::GetHostGlobals()->GetVarTracker()->
      TrackSharedMemoryHandle(instance, host_shm_handle, size);

  base::PlatformFile host_handle =
#if defined(OS_WIN)
      host_shm_handle;
#elif defined(OS_POSIX)
      host_shm_handle.fd;
#else
  #error Not implemented.
#endif
  // We set auto_close to false since we need our file descriptor to
  // actually be duplicated on linux. The shared memory destructor will
  // close the original handle for us.
  plugin_handle->set_shmem(
      host_->ShareHandleWithRemote(host_handle, false), size);
}

}  // namespace chrome
