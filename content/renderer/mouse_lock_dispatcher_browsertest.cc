// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "content/common/view_messages.h"
#include "content/renderer/mouse_lock_dispatcher.h"
#include "content/renderer/render_view_impl.h"
#include "content/test/render_view_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace {

class MockLockTarget : public MouseLockDispatcher::LockTarget {
 public:
   MOCK_METHOD1(OnLockMouseACK, void(bool));
   MOCK_METHOD0(OnMouseLockLost, void());
   MOCK_METHOD1(HandleMouseLockedInputEvent,
                bool(const WebKit::WebMouseEvent&));
};

// MouseLockDispatcher is a RenderViewObserver, and we test it by creating a
// fixture containing a RenderViewImpl view() and interacting to that interface.
class MouseLockDispatcherTest
    : public content::RenderViewTest {
 public:
  virtual void SetUp() {
    content::RenderViewTest::SetUp();
    route_id_ = view()->GetRoutingId();
    target_ = new MockLockTarget();
    alternate_target_ = new MockLockTarget();
  }

  virtual void TearDown() {
    content::RenderViewTest::TearDown();
    delete target_;
    delete alternate_target_;
  }

 protected:
  RenderViewImpl* view() { return static_cast<RenderViewImpl*>(view_); }
  MouseLockDispatcher* dispatcher() { return view()->mouse_lock_dispatcher(); }
  int route_id_;
  MockLockTarget* target_;
  MockLockTarget* alternate_target_;
};

}  // namespace

// Test simple use of RenderViewImpl interface to WebKit for pointer lock.
TEST_F(MouseLockDispatcherTest, BasicWebWidget) {
  // Start unlocked.
  EXPECT_FALSE(view()->isPointerLocked());

  // Lock.
  EXPECT_TRUE(view()->requestPointerLock());
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
  EXPECT_TRUE(view()->isPointerLocked());

  // Unlock.
  view()->requestPointerUnlock();
  view()->OnMessageReceived(ViewMsg_MouseLockLost(route_id_));
  EXPECT_FALSE(view()->isPointerLocked());

  // Attempt a lock, and have it fail.
  EXPECT_TRUE(view()->requestPointerLock());
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, false));
  EXPECT_FALSE(view()->isPointerLocked());
}

// Test simple use of MouseLockDispatcher with a mock LockTarget.
TEST_F(MouseLockDispatcherTest, BasicMockLockTarget) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, OnLockMouseACK(true));
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_));
  EXPECT_CALL(*target_, OnMouseLockLost());
  EXPECT_CALL(*target_, OnLockMouseACK(false));

  // Start unlocked.
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(NULL));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));

  // Lock.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
  EXPECT_TRUE(dispatcher()->IsMouseLockedTo(target_));

  // Receive mouse event.
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());

  // Unlock.
  dispatcher()->UnlockMouse(target_);
  view()->OnMessageReceived(ViewMsg_MouseLockLost(route_id_));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));

  // Attempt a lock, and have it fail.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, false));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));
}

// Test deleting a target while it is in use by MouseLockDispatcher.
TEST_F(MouseLockDispatcherTest, DeleteAndUnlock) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, OnLockMouseACK(true));
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_)).Times(0);
  EXPECT_CALL(*target_, OnMouseLockLost()).Times(0);

  // Lock.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
  EXPECT_TRUE(dispatcher()->IsMouseLockedTo(target_));

  // Unlock, with a deleted target.
  // Don't receive mouse events or lock lost.
  dispatcher()->OnLockTargetDestroyed(target_);
  delete target_;
  target_ = NULL;
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());
  view()->OnMessageReceived(ViewMsg_MouseLockLost(route_id_));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));
}

// Test deleting a target that is pending a lock request response.
TEST_F(MouseLockDispatcherTest, DeleteWithPendingLockSuccess) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, OnLockMouseACK(true)).Times(0);
  EXPECT_CALL(*target_, OnMouseLockLost()).Times(0);

  // Lock request.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));

  // Before receiving response delete the target.
  dispatcher()->OnLockTargetDestroyed(target_);
  delete target_;
  target_ = NULL;

  // Lock response.
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
}

// Test deleting a target that is pending a lock request failure response.
TEST_F(MouseLockDispatcherTest, DeleteWithPendingLockFail) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, OnLockMouseACK(true)).Times(0);
  EXPECT_CALL(*target_, OnMouseLockLost()).Times(0);

  // Lock request.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));

  // Before receiving response delete the target.
  dispatcher()->OnLockTargetDestroyed(target_);
  delete target_;
  target_ = NULL;

  // Lock response.
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, false));
}

// Test not receiving mouse events when a target is not locked.
TEST_F(MouseLockDispatcherTest, MouseEventsNotReceived) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_)).Times(0);
  EXPECT_CALL(*target_, OnLockMouseACK(true));
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_));
  EXPECT_CALL(*target_, OnMouseLockLost());
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_)).Times(0);

  // (Don't) receive mouse event.
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());

  // Lock.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
  EXPECT_TRUE(dispatcher()->IsMouseLockedTo(target_));

  // Receive mouse event.
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());

  // Unlock.
  dispatcher()->UnlockMouse(target_);
  view()->OnMessageReceived(ViewMsg_MouseLockLost(route_id_));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));

  // (Don't) receive mouse event.
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());
}

// Test multiple targets
TEST_F(MouseLockDispatcherTest, MultipleTargets) {
  ::testing::InSequence expect_calls_in_sequence;
  EXPECT_CALL(*target_, OnLockMouseACK(true));
  EXPECT_CALL(*target_, HandleMouseLockedInputEvent(_));
  EXPECT_CALL(*alternate_target_, HandleMouseLockedInputEvent(_)).Times(0);
  EXPECT_CALL(*target_, OnMouseLockLost()).Times(0);
  EXPECT_CALL(*alternate_target_, OnMouseLockLost()).Times(0);
  EXPECT_CALL(*target_, OnMouseLockLost());

  // Lock request for target.
  EXPECT_TRUE(dispatcher()->LockMouse(target_));

  // Fail attempt to lock alternate.
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(alternate_target_));
  EXPECT_FALSE(dispatcher()->LockMouse(alternate_target_));

  // Lock completion for target.
  view()->OnMessageReceived(ViewMsg_LockMouse_ACK(route_id_, true));
  EXPECT_TRUE(dispatcher()->IsMouseLockedTo(target_));

  // Fail attempt to lock alternate.
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(alternate_target_));
  EXPECT_FALSE(dispatcher()->LockMouse(alternate_target_));

  // Receive mouse event to only one target.
  dispatcher()->WillHandleMouseEvent(WebKit::WebMouseEvent());

  // Unlock alternate target has no effect.
  dispatcher()->UnlockMouse(alternate_target_);
  EXPECT_TRUE(dispatcher()->IsMouseLockedTo(target_));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(alternate_target_));

  // Though the call to UnlockMouse should not unlock any target, we will
  // cause an unlock (as if e.g. user escaped mouse lock) and verify the
  // correct target is unlocked.
  view()->OnMessageReceived(ViewMsg_MouseLockLost(route_id_));
  EXPECT_FALSE(dispatcher()->IsMouseLockedTo(target_));
}

