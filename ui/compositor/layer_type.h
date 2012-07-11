// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_LAYER_TYPE_H_
#define UI_COMPOSITOR_LAYER_TYPE_H_

namespace ui {

enum LayerType {
  // A layer that has no onscreen representation (note that its children will
  // still be drawn, though).
  LAYER_NOT_DRAWN = 0,

  // A layer that has a texture.
  LAYER_TEXTURED = 1,

  // A layer that's drawn as a single color.
  LAYER_SOLID_COLOR = 2,
};

}  // namespace ui

#endif  // UI_COMPOSITOR_LAYER_TYPE_H_
