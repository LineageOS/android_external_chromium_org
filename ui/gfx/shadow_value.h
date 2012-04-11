// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SHADOW_VALUE_
#define UI_GFX_SHADOW_VALUE_
#pragma once

#include <string>
#include <vector>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ui_export.h"
#include "ui/gfx/point.h"

namespace gfx {

class Insets;

// ShadowValue encapsulates parameters needed to define a shadow, including the
// shadow's offset, blur amount and color.
class UI_EXPORT ShadowValue {
 public:
  ShadowValue();
  ShadowValue(const gfx::Point& offset, double blur, SkColor color);
  ~ShadowValue();

  int x() const { return offset_.x(); }
  int y() const { return offset_.y(); }
  const gfx::Point& offset() const { return offset_; }
  double blur() const { return blur_; }
  SkColor color() const { return color_; }

  std::string ToString() const;

  // Gets margin space needed for shadows. Note that values in returned Insets
  // are negative because shadow margins are outside a boundary.
  static Insets GetMargin(const std::vector<ShadowValue>& shadows);

 private:
  gfx::Point offset_;

  // Blur amount of the shadow in pixels. If underlying implementation supports
  // (e.g. Skia), it can have fraction part such as 0.5 pixel. The value
  // defines a range from full shadow color at the start point inside the
  // shadow to fully transparent at the end point outside it. The range is
  // perpendicular to and centered on the shadow edge. For example, a blur
  // amount of 4.0 means to have a blurry shadow edge of 4 pixels that
  // transitions from full shadow color to fully transparent and with 2 pixels
  // inside the shadow and 2 pixels goes beyond the edge.
  double blur_;

  SkColor color_;
};

}  // namespace gfx

#endif  // UI_GFX_SHADOW_VALUE_
