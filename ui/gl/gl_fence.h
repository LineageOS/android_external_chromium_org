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

  static GLFence* Create();
  virtual bool HasCompleted() = 0;

 protected:
  static bool IsContextLost();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLFence);
};

}  // namespace gfx

#endif  // UI_GL_GL_FENCE_H_
