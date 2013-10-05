// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_control_service.h"

#include "gpu/command_buffer/client/gpu_memory_buffer_factory.h"
#include "gpu/command_buffer/service/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"

namespace gpu {

GpuControlService::GpuControlService(
    GpuMemoryBufferManagerInterface* gpu_memory_buffer_manager,
    GpuMemoryBufferFactory* gpu_memory_buffer_factory,
    gles2::MailboxManager* mailbox_manager)
    : gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      gpu_memory_buffer_factory_(gpu_memory_buffer_factory),
      mailbox_manager_(mailbox_manager) {
}

GpuControlService::~GpuControlService() {
}

bool GpuControlService::SupportsGpuMemoryBuffer() {
  return gpu_memory_buffer_manager_ && gpu_memory_buffer_factory_;
}

gfx::GpuMemoryBuffer* GpuControlService::CreateGpuMemoryBuffer(
    size_t width,
    size_t height,
    unsigned internalformat,
    int32* id) {
  *id = -1;

  CHECK(gpu_memory_buffer_factory_) << "No GPU memory buffer factory provided";
  linked_ptr<gfx::GpuMemoryBuffer> buffer = make_linked_ptr(
      gpu_memory_buffer_factory_->CreateGpuMemoryBuffer(width,
                                                        height,
                                                        internalformat));
  if (!buffer.get())
    return NULL;

  static int32 next_id = 1;
  *id = next_id++;

  if (!RegisterGpuMemoryBuffer(*id,
                               buffer->GetHandle(),
                               width,
                               height,
                               internalformat)) {
    *id = -1;
    return NULL;
  }

  gpu_memory_buffers_[*id] = buffer;
  return buffer.get();
}

void GpuControlService::DestroyGpuMemoryBuffer(int32 id) {
  GpuMemoryBufferMap::iterator it = gpu_memory_buffers_.find(id);
  if (it != gpu_memory_buffers_.end())
    gpu_memory_buffers_.erase(it);

  gpu_memory_buffer_manager_->DestroyGpuMemoryBuffer(id);
}

uint32 GpuControlService::InsertSyncPoint() {
  NOTREACHED();
  return 0u;
}

void GpuControlService::SignalSyncPoint(uint32 sync_point,
                                        const base::Closure& callback) {
  NOTREACHED();
}

void GpuControlService::SignalQuery(uint32 query,
                                    const base::Closure& callback) {
  NOTREACHED();
}

bool GpuControlService::RegisterGpuMemoryBuffer(
    int32 id,
    gfx::GpuMemoryBufferHandle buffer,
    size_t width,
    size_t height,
    unsigned internalformat) {
  return gpu_memory_buffer_manager_->RegisterGpuMemoryBuffer(id,
                                                             buffer,
                                                             width,
                                                             height,
                                                             internalformat);
}

bool GpuControlService::GenerateMailboxNames(
    unsigned num, std::vector<gpu::Mailbox>* names) {
  DCHECK(names->empty());
  names->resize(num);
  for (unsigned i = 0; i < num; ++i) {
    gles2::MailboxName name;
    mailbox_manager_->GenerateMailboxName(&name);
    (*names)[i].SetName(name.key);
  }
  return true;
}

}  // namespace gpu
