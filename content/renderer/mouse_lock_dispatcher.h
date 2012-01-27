// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_
#define CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_
#pragma once

#include "base/basictypes.h"
#include "content/public/renderer/render_view_observer.h"

class RenderViewImpl;

namespace WebKit {
class WebMouseEvent;
class WebWidget;
}  // namespace WebKit

namespace webkit{
namespace ppapi {
class PluginInstance;
}  // namespace ppapi
}  // namespace webkit

// MouseLockDispatcher is owned by RenderViewImpl.
class CONTENT_EXPORT MouseLockDispatcher : public content::RenderViewObserver {
 public:
  explicit MouseLockDispatcher(RenderViewImpl* render_view_impl);
  virtual ~MouseLockDispatcher();

  class LockTarget {
   public:
    virtual ~LockTarget() {}
    // A mouse lock request was pending and this reports success or failure.
    virtual void OnLockMouseACK(bool succeeded) = 0;
    // A mouse lock was in place, but has been lost.
    virtual void OnMouseLockLost() = 0;
    // A mouse lock is enabled and mouse events are being delievered.
    virtual bool HandleMouseLockedInputEvent(
        const WebKit::WebMouseEvent& event) = 0;
  };

  // Locks the mouse to the |target|. If true is returned, an asynchronous
  // response to target->OnLockMouseACK() will follow.
  bool LockMouse(LockTarget* target);
  // Request to unlock the mouse. An asynchronous response to
  // target->OnMouseLockLost() will follow.
  void UnlockMouse(LockTarget* target);
  // Clears out the reference to the |target| because it has or is being
  // destroyed. Unlocks if locked. The pointer will not be accessed.
  void OnLockTargetDestroyed(LockTarget* target);
  bool IsMouseLockedTo(LockTarget* target);

  // Allow lock target to consumed a mouse event, if it does return true.
  bool WillHandleMouseEvent(const WebKit::WebMouseEvent& event);

 private:
  // RenderView::Observer implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // IPC handlers.
  void OnLockMouseACK(bool succeeded);
  void OnMouseLockLost();

  bool MouseLockedOrPendingAction() const {
    return mouse_locked_ || pending_lock_request_ || pending_unlock_request_;
  }

  RenderViewImpl* render_view_impl_;

  bool mouse_locked_;
  // If both |pending_lock_request_| and |pending_unlock_request_| are true,
  // it means a lock request was sent before an unlock request and we haven't
  // received responses for them. The logic in LockMouse() makes sure that a
  // lock request won't be sent when there is a pending unlock request.
  bool pending_lock_request_;
  bool pending_unlock_request_;

  // |target_| is the pending or current owner of mouse lock. We retain a non
  // owning reference here that must be cleared by |OnLockTargetDestroyed|
  // when it is destroyed.
  LockTarget* target_;

  DISALLOW_COPY_AND_ASSIGN(MouseLockDispatcher);
};

#endif  // CONTENT_RENDERER_MOUSE_LOCK_DISPATCHER_H_
