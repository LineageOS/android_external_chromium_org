// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_WRAPPER_H_
#define UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_WRAPPER_H_

#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"

namespace views {

class NativeViewHost;

// An interface that implemented by an object that wraps a gfx::NativeView on
// a specific platform, used to perform platform specific operations on that
// native view when attached, detached, moved and sized.
class VIEWS_EXPORT NativeViewHostWrapper {
 public:
  virtual ~NativeViewHostWrapper() {}

  // Called at the end of NativeViewHost::Attach, allowing the wrapper to
  // perform platform-specific operations that need to occur to complete
  // attaching the gfx::NativeView.
  virtual void AttachNativeView() = 0;

  // Called before the attached gfx::NativeView is detached from the
  // NativeViewHost, allowing the wrapper to perform platform-specific
  // cleanup. |destroyed| is true if the native view is detached
  // because it's being destroyed, or false otherwise.
  virtual void NativeViewDetaching(bool destroyed) = 0;

  // Called when our associated NativeViewHost is added to a View hierarchy
  // rooted at a valid Widget.
  virtual void AddedToWidget() = 0;

  // Called when our associated NativeViewHost is removed from a View hierarchy
  // rooted at a valid Widget.
  virtual void RemovedFromWidget() = 0;

  // Installs a clip on the gfx::NativeView. These values are in the coordinate
  // space of the Widget, so if this method is called from ShowWidget
  // then the values need to be translated.
  virtual void InstallClip(int x, int y, int w, int h) = 0;

  // Whether or not a clip has been installed on the wrapped gfx::NativeView.
  virtual bool HasInstalledClip() = 0;

  // Removes the clip installed on the gfx::NativeView by way of InstallClip. A
  // following call to ShowWidget should occur after calling this method to
  // position the gfx::NativeView correctly, since the clipping process may have
  // adjusted its position.
  virtual void UninstallClip() = 0;

  // Shows the gfx::NativeView at the specified position (relative to the parent
  // native view).
  virtual void ShowWidget(int x, int y, int w, int h) = 0;

  // Hides the gfx::NativeView. NOTE: this may be invoked when the native view
  // is already hidden.
  virtual void HideWidget() = 0;

  // Sets focus to the gfx::NativeView.
  virtual void SetFocus() = 0;

  // Return the native view accessible corresponding to the wrapped native
  // view.
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() = 0;

  // Returns the native cursor corresponding to the point (x, y)
  // in the native view.
  virtual gfx::NativeCursor GetCursor(int x, int y) = 0;

  // Creates a platform-specific instance of an object implementing this
  // interface.
  static NativeViewHostWrapper* CreateWrapper(NativeViewHost* host);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_WRAPPER_H_
