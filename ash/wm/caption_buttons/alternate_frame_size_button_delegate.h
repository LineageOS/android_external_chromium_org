// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_CAPTION_BUTTONS_ALTERNATE_FRAME_SIZE_BUTTON_DELEGATE_H_
#define ASH_WM_CAPTION_BUTTONS_ALTERNATE_FRAME_SIZE_BUTTON_DELEGATE_H_

#include "ash/ash_export.h"
#include "ash/wm/caption_buttons/caption_button_types.h"

namespace gfx {
class Insets;
class Point;
class Vector2d;
}

namespace ash {
class FrameCaptionButton;

// Delegate interface for AlternateFrameSizeButton.
class ASH_EXPORT AlternateFrameSizeButtonDelegate {
 public:
  enum Animate {
    ANIMATE_YES,
    ANIMATE_NO
  };

  // Returns whether the minimize button is visible.
  virtual bool IsMinimizeButtonVisible() const = 0;

  // Reset the caption button views::Button::ButtonState back to normal. If
  // |animate| is ANIMATE_YES, the buttons will crossfade back to their
  // original icons.
  virtual void SetButtonsToNormal(Animate animate) = 0;

  // Sets the minimize and close button icons. The buttons will crossfade to
  // their new icons if |animate| is ANIMATE_YES.
  virtual void SetButtonIcons(CaptionButtonIcon minimize_button_icon,
                              CaptionButtonIcon close_button_icon,
                              Animate animate) = 0;

  // Returns the button closest to |position_in_screen|.
  virtual const FrameCaptionButton* GetButtonClosestTo(
      const gfx::Point& position_in_screen) const = 0;

  // Sets |to_hover| and |to_pressed| to STATE_HOVERED and STATE_PRESSED
  // respectively. All other buttons are to set to STATE_NORMAL.
  virtual void SetHoveredAndPressedButtons(
      const FrameCaptionButton* to_hover,
      const FrameCaptionButton* to_press) = 0;

 protected:
  virtual ~AlternateFrameSizeButtonDelegate() {}
};

}  // namespace ash

#endif  // ASH_WM_CAPTION_BUTTONS_ALTERNATE_FRAME_SIZE_BUTTON_DELEGATE_H_
