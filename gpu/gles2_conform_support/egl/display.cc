// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/gles2_conform_support/egl/display.h"

#include <vector>
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "gpu/command_buffer/client/gles2_lib.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/gles2_conform_support/egl/config.h"
#include "gpu/gles2_conform_support/egl/surface.h"

namespace {
const int32 kCommandBufferSize = 1024 * 1024;
const int32 kTransferBufferSize = 512 * 1024;
}

namespace egl {

Display::Display(EGLNativeDisplayType display_id)
    : display_id_(display_id),
      is_initialized_(false) {
}

Display::~Display() {
  gles2::Terminate();
}

bool Display::Initialize() {
  gles2::Initialize();
  is_initialized_ = true;
  return true;
}

bool Display::IsValidConfig(EGLConfig config) {
  return (config != NULL) && (config == config_.get());
}

bool Display::GetConfigs(EGLConfig* configs,
                         EGLint config_size,
                         EGLint* num_config) {
  // TODO(alokp): Find out a way to find all configs. CommandBuffer currently
  // does not support finding or choosing configs.
  *num_config = 1;
  if (configs != NULL) {
    if (config_ == NULL) {
      config_.reset(new Config);
    }
    configs[0] = config_.get();
  }
  return true;
}

bool Display::GetConfigAttrib(EGLConfig config,
                              EGLint attribute,
                              EGLint* value) {
  const egl::Config* cfg = static_cast<egl::Config*>(config);
  return cfg->GetAttrib(attribute, value);
}

bool Display::IsValidNativeWindow(EGLNativeWindowType win) {
#if defined OS_WIN
  return ::IsWindow(win) != FALSE;
#else
  // TODO(alokp): Validate window handle.
  return true;
#endif  // OS_WIN
}

bool Display::IsValidSurface(EGLSurface surface) {
  return (surface != NULL) && (surface == surface_.get());
}

EGLSurface Display::CreateWindowSurface(EGLConfig config,
                                        EGLNativeWindowType win,
                                        const EGLint* attrib_list) {
  if (surface_ != NULL) {
    // We do not support more than one window surface.
    return EGL_NO_SURFACE;
  }

  scoped_ptr<gpu::CommandBufferService> command_buffer(
      new gpu::CommandBufferService);
  if (!command_buffer->Initialize())
    return false;

  gpu::gles2::ContextGroup::Ref group(new gpu::gles2::ContextGroup(true));

  decoder_.reset(gpu::gles2::GLES2Decoder::Create(group.get()));
  if (!decoder_.get())
    return EGL_NO_SURFACE;

  gpu_scheduler_.reset(new gpu::GpuScheduler(command_buffer.get(),
                                             decoder_.get(),
                                             NULL));

  decoder_->set_engine(gpu_scheduler_.get());

  gl_surface_ = gfx::GLSurface::CreateViewGLSurface(false, win);
  if (!gl_surface_.get())
    return EGL_NO_SURFACE;

  gl_context_ = gfx::GLContext::CreateGLContext(NULL,
                                                gl_surface_.get(),
                                                gfx::PreferDiscreteGpu);
  if (!gl_context_.get())
    return EGL_NO_SURFACE;

  std::vector<int32> attribs;
  if (!decoder_->Initialize(gl_surface_.get(),
                            gl_context_.get(),
                            gfx::Size(),
                            gpu::gles2::DisallowedFeatures(),
                            NULL,
                            attribs)) {
    return EGL_NO_SURFACE;
  }

  command_buffer->SetPutOffsetChangeCallback(
      base::Bind(&gpu::GpuScheduler::PutChanged,
                 base::Unretained(gpu_scheduler_.get())));
  command_buffer->SetGetBufferChangeCallback(
      base::Bind(&gpu::GpuScheduler::SetGetBuffer,
                 base::Unretained(gpu_scheduler_.get())));

  scoped_ptr<gpu::gles2::GLES2CmdHelper> cmd_helper(
      new gpu::gles2::GLES2CmdHelper(command_buffer.get()));
  if (!cmd_helper->Initialize(kCommandBufferSize))
    return false;

  scoped_ptr<gpu::TransferBuffer> transfer_buffer(new gpu::TransferBuffer(
      cmd_helper.get()));

  command_buffer_.reset(command_buffer.release());
  transfer_buffer_.reset(transfer_buffer.release());
  gles2_cmd_helper_.reset(cmd_helper.release());
  surface_.reset(new Surface(win));

  return surface_.get();
}

void Display::DestroySurface(EGLSurface surface) {
  DCHECK(IsValidSurface(surface));
  gpu_scheduler_.reset();
  if (decoder_.get()) {
    decoder_->Destroy();
  }
  decoder_.reset();
  gl_surface_ = NULL;
  gl_context_ = NULL;
  surface_.reset();
}

void Display::SwapBuffers(EGLSurface surface) {
  DCHECK(IsValidSurface(surface));
  context_->SwapBuffers();
}

bool Display::IsValidContext(EGLContext ctx) {
  return (ctx != NULL) && (ctx == context_.get());
}

EGLContext Display::CreateContext(EGLConfig config,
                                  EGLContext share_ctx,
                                  const EGLint* attrib_list) {
  DCHECK(IsValidConfig(config));
  // TODO(alokp): Add support for shared contexts.
  if (share_ctx != NULL)
    return EGL_NO_CONTEXT;

  DCHECK(command_buffer_ != NULL);
  DCHECK(transfer_buffer_.get());
  bool share_resources = share_ctx != NULL;
  context_.reset(new gpu::gles2::GLES2Implementation(
      gles2_cmd_helper_.get(),
      transfer_buffer_.get(),
      share_resources,
      true));

  if (!context_->Initialize(
      kTransferBufferSize,
      kTransferBufferSize / 2,
      kTransferBufferSize * 2)) {
    return EGL_NO_CONTEXT;
  }

  context_->EnableFeatureCHROMIUM("pepper3d_allow_buffers_on_multiple_targets");
  context_->EnableFeatureCHROMIUM("pepper3d_support_fixed_attribs");

  return context_.get();
}

void Display::DestroyContext(EGLContext ctx) {
  DCHECK(IsValidContext(ctx));
  context_.reset();
  transfer_buffer_.reset();
}

bool Display::MakeCurrent(EGLSurface draw, EGLSurface read, EGLContext ctx) {
  if (ctx == EGL_NO_CONTEXT) {
    gles2::SetGLContext(NULL);
  } else {
    DCHECK(IsValidSurface(draw));
    DCHECK(IsValidSurface(read));
    DCHECK(IsValidContext(ctx));
    gles2::SetGLContext(context_.get());
  }
  return true;
}

}  // namespace egl
