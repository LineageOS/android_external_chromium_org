// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_IMAGE_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_IMAGE_TRANSPORT_FACTORY_H_
#pragma once

#include "base/memory/ref_counted.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Size;
class ScopedMakeCurrent;
}

namespace ui {
class Compositor;
class ContextFactory;
class Texture;
}

class ImageTransportClient;

// This class provides the interface for creating the support for the
// cross-process image transport, both for creating the shared surface handle
// (destination surface for the GPU process) and the transport client (logic for
// using that surface as a texture). The factory is a process-wide singleton.
// As this is intimately linked to the type of 3D context we use (in-process or
// command-buffer), implementations of this also implement ui::ContextFactory.
class ImageTransportFactory {
 public:
  virtual ~ImageTransportFactory() { }

  // Initialize the global transport factory.
  static void Initialize();

  // Terminates the global transport factory.
  static void Terminate();

  // Gets the factory instance.
  static ImageTransportFactory* GetInstance();

  // Gets the image transport factory as a context factory for the compositor.
  virtual ui::ContextFactory* AsContextFactory() = 0;

  // Creates a shared surface handle, associated with the compositor.
  virtual gfx::GLSurfaceHandle CreateSharedSurfaceHandle(
      ui::Compositor* compositor) = 0;

  // Destroys a shared surface handle.
  virtual void DestroySharedSurfaceHandle(gfx::GLSurfaceHandle surface) = 0;

  // Creates a transport client of a given size, and using the opaque handle
  // sent by the GPU process.
  virtual scoped_refptr<ImageTransportClient> CreateTransportClient(
      const gfx::Size& size,
      uint64* transport_handle) = 0;

  // Returns a ScopedMakeCurrent that can be used to make current a context that
  // is shared with the compositor context, e.g. to create a texture in its
  // namespace. The caller gets ownership of the object.
  // This will return NULL when using out-of-process (command buffer) contexts.
  virtual gfx::ScopedMakeCurrent* GetScopedMakeCurrent() = 0;
};

#endif  // CONTENT_BROWSER_RENDERER_HOST_IMAGE_TRANSPORT_FACTORY_H_
