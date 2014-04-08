// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_IMAGE_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_IMAGE_TRANSPORT_FACTORY_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Size;
}

namespace ui {
class ContextFactory;
class Texture;
}

namespace blink {
class WebGraphicsContext3D;
}

namespace content {
class GLHelper;

// This class provides a way to get notified when surface handles get lost.
class CONTENT_EXPORT ImageTransportFactoryObserver {
 public:
  virtual ~ImageTransportFactoryObserver() {}

  // Notifies that the surface handles generated by ImageTransportFactory were
  // lost.
  // When this is called, the old resources (e.g. shared context, GL helper)
  // still exist, but are about to be destroyed. Getting a reference to those
  // resources from the ImageTransportFactory (e.g. through GetGLHelper) will
  // return newly recreated, valid resources.
  virtual void OnLostResources() = 0;
};

// This class provides the interface for creating the support for the
// cross-process image transport, both for creating the shared surface handle
// (destination surface for the GPU process) and the transport client (logic for
// using that surface as a texture). The factory is a process-wide singleton.
class CONTENT_EXPORT ImageTransportFactory {
 public:
  virtual ~ImageTransportFactory() {}

  // Initializes the global transport factory.
  static void Initialize();

  // Initializes the global transport factory for unit tests using the provided
  // context factory.
  static void InitializeForUnitTests(
      scoped_ptr<ui::ContextFactory> test_factory);

  // Terminates the global transport factory.
  static void Terminate();

  // Gets the factory instance.
  static ImageTransportFactory* GetInstance();

  // Gets the image transport factory as a context factory for the compositor.
  virtual ui::ContextFactory* AsContextFactory() = 0;

  virtual gfx::GLSurfaceHandle GetSharedSurfaceHandle() = 0;

  // Creates a ui::Texture that deletes the GL texture when deleted.
  virtual scoped_refptr<ui::Texture> CreateOwnedTexture(
      const gfx::Size& size,
      float device_scale_factor,
      unsigned int texture_id) = 0;

  // Gets a GLHelper instance, associated with the shared context. This
  // GLHelper will get destroyed whenever the shared context is lost
  // (ImageTransportFactoryObserver::OnLostResources is called).
  virtual GLHelper* GetGLHelper() = 0;

  virtual void AddObserver(ImageTransportFactoryObserver* observer) = 0;
  virtual void RemoveObserver(ImageTransportFactoryObserver* observer) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_IMAGE_TRANSPORT_FACTORY_H_
