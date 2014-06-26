// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/panels/attached_panel_window_targeter.h"

#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/wm/panels/panel_layout_manager.h"

namespace ash {

AttachedPanelWindowTargeter::AttachedPanelWindowTargeter(
    aura::Window* container,
    const gfx::Insets& default_mouse_extend,
    const gfx::Insets& default_touch_extend,
    PanelLayoutManager* panel_layout_manager)
    : ::wm::EasyResizeWindowTargeter(container,
                                     default_mouse_extend,
                                     default_touch_extend),
      panel_container_(container),
      panel_layout_manager_(panel_layout_manager),
      default_touch_extend_(default_touch_extend) {
  Shell::GetInstance()->AddShellObserver(this);
}

AttachedPanelWindowTargeter::~AttachedPanelWindowTargeter() {
  Shell::GetInstance()->RemoveShellObserver(this);
}

void AttachedPanelWindowTargeter::OnShelfCreatedForRootWindow(
    aura::Window* root_window) {
  UpdateTouchExtend(root_window);
}

void AttachedPanelWindowTargeter::OnShelfAlignmentChanged(
    aura::Window* root_window) {
  // Don't update the touch insets if the shelf has not yet been created.
  if (!panel_layout_manager_->shelf())
    return;

  UpdateTouchExtend(root_window);
}

void AttachedPanelWindowTargeter::UpdateTouchExtend(aura::Window* root_window) {
  // Only update the touch insets for panels if they are attached to the shelf
  // in |root_window|.
  if (panel_container_->GetRootWindow() != root_window)
    return;

  DCHECK(panel_layout_manager_->shelf());

  gfx::Insets touch(default_touch_extend_);
  switch (panel_layout_manager_->shelf()->alignment()) {
    case SHELF_ALIGNMENT_BOTTOM:
      touch = gfx::Insets(touch.top(), touch.left(), 0, touch.right());
      break;
    case SHELF_ALIGNMENT_LEFT:
      touch = gfx::Insets(touch.top(), 0, touch.bottom(), touch.right());
      break;
    case SHELF_ALIGNMENT_RIGHT:
      touch = gfx::Insets(touch.top(), touch.left(), touch.bottom(), 0);
      break;
    case SHELF_ALIGNMENT_TOP:
      touch = gfx::Insets(0, touch.left(), touch.bottom(), touch.right());
      break;
    default:
      NOTREACHED();
      return;
  }

  set_touch_extend(touch);
}

}  // namespace ash
