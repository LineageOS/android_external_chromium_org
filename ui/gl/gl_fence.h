// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_FENCE_H_
#define UI_GL_GL_FENCE_H_

#include "base/basictypes.h"
#include "ui/gl/gl_export.h"

namespace gfx {

class GL_EXPORT GLFence {
 public:
  GLFence();
  virtual ~GLFence();

  static bool IsSupported();
  static GLFence* Create();

  // Creates a fence that is not guaranteed to signal until the current context
  // is flushed. It is illegal to call Client/ServerWait() on a fence without
  // having explicitly called glFlush() or glFinish() in the originating
  // context.
  static GLFence* CreateWithoutFlush();

  virtual bool HasCompleted() = 0;
  virtual void ClientWait() = 0;

  // Will block the server if supported, but might fall back to blocking the
  // client.
  virtual void ServerWait() = 0;

 protected:
  static bool IsContextLost();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLFence);
};

}  // namespace gfx

#endif  // UI_GL_GL_FENCE_H_
