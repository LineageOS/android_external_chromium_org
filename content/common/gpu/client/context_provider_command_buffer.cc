// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/client/context_provider_command_buffer.h"

#include <set>
#include <vector>

#include "base/callback_helpers.h"
#include "base/strings/stringprintf.h"
#include "cc/output/managed_memory_policy.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "webkit/common/gpu/grcontext_for_webgraphicscontext3d.h"

namespace content {

class ContextProviderCommandBuffer::LostContextCallbackProxy
    : public blink::WebGraphicsContext3D::WebGraphicsContextLostCallback {
 public:
  explicit LostContextCallbackProxy(ContextProviderCommandBuffer* provider)
      : provider_(provider) {
    provider_->context3d_->setContextLostCallback(this);
  }

  virtual ~LostContextCallbackProxy() {
    provider_->context3d_->setContextLostCallback(NULL);
  }

  virtual void onContextLost() {
    provider_->OnLostContext();
  }

 private:
  ContextProviderCommandBuffer* provider_;
};

class ContextProviderCommandBuffer::SwapBuffersCompleteCallbackProxy
    : public blink::WebGraphicsContext3D::
          WebGraphicsSwapBuffersCompleteCallbackCHROMIUM {
 public:
  explicit SwapBuffersCompleteCallbackProxy(
      ContextProviderCommandBuffer* provider)
      : provider_(provider) {
    provider_->context3d_->setSwapBuffersCompleteCallbackCHROMIUM(this);
  }

  virtual ~SwapBuffersCompleteCallbackProxy() {
    provider_->context3d_->setSwapBuffersCompleteCallbackCHROMIUM(NULL);
  }

  virtual void onSwapBuffersComplete() {
    provider_->OnSwapBuffersComplete();
  }

 private:
  ContextProviderCommandBuffer* provider_;
};

scoped_refptr<ContextProviderCommandBuffer>
ContextProviderCommandBuffer::Create(
    scoped_ptr<WebGraphicsContext3DCommandBufferImpl> context3d,
    const std::string& debug_name) {
  if (!context3d)
    return NULL;

  return new ContextProviderCommandBuffer(context3d.Pass(), debug_name);
}

ContextProviderCommandBuffer::ContextProviderCommandBuffer(
    scoped_ptr<WebGraphicsContext3DCommandBufferImpl> context3d,
    const std::string& debug_name)
    : context3d_(context3d.Pass()),
      debug_name_(debug_name),
      leak_on_destroy_(false),
      destroyed_(false) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK(context3d_);
  context_thread_checker_.DetachFromThread();
}

ContextProviderCommandBuffer::~ContextProviderCommandBuffer() {
  DCHECK(main_thread_checker_.CalledOnValidThread() ||
         context_thread_checker_.CalledOnValidThread());

  base::AutoLock lock(main_thread_lock_);

  // Destroy references to the context3d_ before leaking it.
  if (context3d_->GetCommandBufferProxy()) {
    context3d_->GetCommandBufferProxy()->SetMemoryAllocationChangedCallback(
        CommandBufferProxyImpl::MemoryAllocationChangedCallback());
  }
  swap_buffers_complete_callback_proxy_.reset();
  lost_context_callback_proxy_.reset();

  if (leak_on_destroy_) {
    WebGraphicsContext3DCommandBufferImpl* context3d ALLOW_UNUSED =
        context3d_.release();
    webkit::gpu::GrContextForWebGraphicsContext3D* gr_context ALLOW_UNUSED =
        gr_context_.release();
  }
}

bool ContextProviderCommandBuffer::BindToCurrentThread() {
  // This is called on the thread the context will be used.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (lost_context_callback_proxy_)
    return true;

  if (!context3d_->makeContextCurrent())
    return false;

  InitializeCapabilities();

  std::string unique_context_name =
      base::StringPrintf("%s-%p", debug_name_.c_str(), context3d_.get());
  context3d_->pushGroupMarkerEXT(unique_context_name.c_str());

  lost_context_callback_proxy_.reset(new LostContextCallbackProxy(this));
  swap_buffers_complete_callback_proxy_.reset(
      new SwapBuffersCompleteCallbackProxy(this));
  context3d_->GetCommandBufferProxy()->SetMemoryAllocationChangedCallback(
      base::Bind(&ContextProviderCommandBuffer::OnMemoryAllocationChanged,
                 base::Unretained(this)));
  return true;
}

WebGraphicsContext3DCommandBufferImpl*
ContextProviderCommandBuffer::Context3d() {
  DCHECK(lost_context_callback_proxy_);  // Is bound to thread.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  return context3d_.get();
}

gpu::ContextSupport* ContextProviderCommandBuffer::ContextSupport() {
  return context3d_->GetContextSupport();
}

class GrContext* ContextProviderCommandBuffer::GrContext() {
  DCHECK(lost_context_callback_proxy_);  // Is bound to thread.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (gr_context_)
    return gr_context_->get();

  gr_context_.reset(
      new webkit::gpu::GrContextForWebGraphicsContext3D(context3d_.get()));
  return gr_context_->get();
}

cc::ContextProvider::Capabilities
ContextProviderCommandBuffer::ContextCapabilities() {
  DCHECK(lost_context_callback_proxy_);  // Is bound to thread.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  return capabilities_;
}

bool ContextProviderCommandBuffer::IsContextLost() {
  DCHECK(lost_context_callback_proxy_);  // Is bound to thread.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  return context3d_->isContextLost();
}

void ContextProviderCommandBuffer::VerifyContexts() {
  DCHECK(lost_context_callback_proxy_);  // Is bound to thread.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (context3d_->isContextLost())
    OnLostContext();
}

void ContextProviderCommandBuffer::OnLostContext() {
  DCHECK(context_thread_checker_.CalledOnValidThread());
  {
    base::AutoLock lock(main_thread_lock_);
    if (destroyed_)
      return;
    destroyed_ = true;
  }
  if (!lost_context_callback_.is_null())
    base::ResetAndReturn(&lost_context_callback_).Run();
}

void ContextProviderCommandBuffer::OnSwapBuffersComplete() {
  DCHECK(context_thread_checker_.CalledOnValidThread());
  if (!swap_buffers_complete_callback_.is_null())
    swap_buffers_complete_callback_.Run();
}

void ContextProviderCommandBuffer::OnMemoryAllocationChanged(
    const gpu::MemoryAllocation& allocation) {
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (gr_context_) {
    bool nonzero_allocation = !!allocation.bytes_limit_when_visible;
    gr_context_->SetMemoryLimit(nonzero_allocation);
  }

  if (memory_policy_changed_callback_.is_null())
    return;

  memory_policy_changed_callback_.Run(cc::ManagedMemoryPolicy(allocation));
}

void ContextProviderCommandBuffer::InitializeCapabilities() {
  // The command buffer provides the following capabilities always.
  // TODO(jamesr): This information is duplicated with
  // gpu::gles2::FeatureInfo::AddFeatures().
  Capabilities caps(context3d_->GetImplementation()->capabilities());
  caps.discard_backbuffer = true;
  caps.set_visibility = true;

  // TODO(jamesr): These are also added in
  // gpu::gles2::GLES2Implementation::GetStringHelper() on the client side.
  caps.map_sub = true;
  caps.shallow_flush = true;

  // The swapbuffers complete callback is always supported by multi-process
  // command buffer implementations.
  caps.swapbuffers_complete_callback = true;

  size_t mapped_memory_limit = context3d_->GetMappedMemoryLimit();
  caps.max_transfer_buffer_usage_bytes =
      mapped_memory_limit == WebGraphicsContext3DCommandBufferImpl::kNoLimit
      ? std::numeric_limits<size_t>::max() : mapped_memory_limit;

  capabilities_ = caps;
}


bool ContextProviderCommandBuffer::DestroyedOnMainThread() {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  base::AutoLock lock(main_thread_lock_);
  return destroyed_;
}

void ContextProviderCommandBuffer::SetLostContextCallback(
    const LostContextCallback& lost_context_callback) {
  DCHECK(context_thread_checker_.CalledOnValidThread());
  DCHECK(lost_context_callback_.is_null() ||
         lost_context_callback.is_null());
  lost_context_callback_ = lost_context_callback;
}

void ContextProviderCommandBuffer::SetSwapBuffersCompleteCallback(
    const SwapBuffersCompleteCallback& swap_buffers_complete_callback) {
  DCHECK(context_thread_checker_.CalledOnValidThread());
  DCHECK(swap_buffers_complete_callback_.is_null() ||
         swap_buffers_complete_callback.is_null());
  swap_buffers_complete_callback_ = swap_buffers_complete_callback;
}

void ContextProviderCommandBuffer::SetMemoryPolicyChangedCallback(
    const MemoryPolicyChangedCallback& memory_policy_changed_callback) {
  DCHECK(context_thread_checker_.CalledOnValidThread());
  DCHECK(memory_policy_changed_callback_.is_null() ||
         memory_policy_changed_callback.is_null());
  memory_policy_changed_callback_ = memory_policy_changed_callback;
}

}  // namespace content
