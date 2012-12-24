// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_GPU_MEMORY_TRACKING_H_
#define CONTENT_COMMON_GPU_GPU_MEMORY_TRACKING_H_

#if defined(ENABLE_GPU)

#include "base/basictypes.h"
#include "base/process.h"
#include "content/common/content_export.h"
#include "gpu/command_buffer/service/memory_tracking.h"

namespace content {

class GpuMemoryManager;

// All decoders in a context group point to a single GpuMemoryTrackingGroup,
// which tracks GPU resource consumption for the entire context group.
class CONTENT_EXPORT GpuMemoryTrackingGroup {
 public:
  ~GpuMemoryTrackingGroup();
  void TrackMemoryAllocatedChange(
      size_t old_size,
      size_t new_size,
      gpu::gles2::MemoryTracker::Pool tracking_pool);
  base::ProcessId GetPid() const {
    return pid_;
  }
  size_t GetSize() const {
    return size_;
  }
  gpu::gles2::MemoryTracker* GetMemoryTracker() const {
    return memory_tracker_;
  }

 private:
  friend class GpuMemoryManager;

  GpuMemoryTrackingGroup(base::ProcessId pid,
                         gpu::gles2::MemoryTracker* memory_tracker,
                         GpuMemoryManager* memory_manager);

  base::ProcessId pid_;
  size_t size_;

  // Set and used only during the Manage function, to determine which
  // non-surface clients should be hibernated.
  bool hibernated_;

  gpu::gles2::MemoryTracker* memory_tracker_;
  GpuMemoryManager* memory_manager_;
};

}  // namespace content

#endif

#endif // CONTENT_COMMON_GPU_GPU_MEMORY_TRACKING_H_
