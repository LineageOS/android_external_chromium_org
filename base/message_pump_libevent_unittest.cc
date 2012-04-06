// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_pump_libevent.h"

#include <unistd.h>

#include "base/message_loop.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(USE_SYSTEM_LIBEVENT)
#include <event.h>
#else
#include "third_party/libevent/event.h"
#endif

namespace base {

class MessagePumpLibeventTest : public testing::Test {
 protected:
  MessagePumpLibeventTest()
      : ui_loop_(MessageLoop::TYPE_UI),
        io_thread_("MessagePumpLibeventTestIOThread") {}
  virtual ~MessagePumpLibeventTest() {}

  virtual void SetUp() OVERRIDE {
    Thread::Options options(MessageLoop::TYPE_IO, 0);
    ASSERT_TRUE(io_thread_.StartWithOptions(options));
    ASSERT_EQ(MessageLoop::TYPE_IO, io_thread_.message_loop()->type());
  }

  MessageLoop* ui_loop() { return &ui_loop_; }
  MessageLoopForIO* io_loop() const {
    return static_cast<MessageLoopForIO*>(io_thread_.message_loop());
  }

  void OnLibeventNotification(
      MessagePumpLibevent* pump,
      MessagePumpLibevent::FileDescriptorWatcher* controller) {
    pump->OnLibeventNotification(0, EV_WRITE | EV_READ, controller);
  }

  MessageLoop ui_loop_;
  Thread io_thread_;
};

namespace {

// Concrete implementation of MessagePumpLibevent::Watcher that does
// nothing useful.
class StupidWatcher : public MessagePumpLibevent::Watcher {
 public:
  virtual ~StupidWatcher() {}

  // base:MessagePumpLibevent::Watcher interface
  virtual void OnFileCanReadWithoutBlocking(int fd) OVERRIDE {}
  virtual void OnFileCanWriteWithoutBlocking(int fd) OVERRIDE {}
};

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)

// Test to make sure that we catch calling WatchFileDescriptor off of the
// wrong thread.
TEST_F(MessagePumpLibeventTest, TestWatchingFromBadThread) {
  MessagePumpLibevent::FileDescriptorWatcher watcher;
  StupidWatcher delegate;

  ASSERT_DEATH(io_loop()->WatchFileDescriptor(
      STDOUT_FILENO, false, MessageLoopForIO::WATCH_READ, &watcher, &delegate),
      "Check failed: "
      "watch_file_descriptor_caller_checker_.CalledOnValidThread()");
}

#endif  // GTEST_HAS_DEATH_TEST && !defined(NDEBUG)

class DeleteWatcher : public MessagePumpLibevent::Watcher {
 public:
  explicit DeleteWatcher(
      MessagePumpLibevent::FileDescriptorWatcher* controller)
      : controller_(controller) {
    DCHECK(controller_);
  }
  virtual ~DeleteWatcher() {}

  // base:MessagePumpLibevent::Watcher interface
  virtual void OnFileCanReadWithoutBlocking(int /* fd */) OVERRIDE {
    NOTREACHED();
  }
  virtual void OnFileCanWriteWithoutBlocking(int /* fd */) OVERRIDE {
    delete controller_;
  }

 private:
  MessagePumpLibevent::FileDescriptorWatcher* const controller_;
};

TEST_F(MessagePumpLibeventTest, DeleteWatcher) {
  scoped_refptr<MessagePumpLibevent> pump(new MessagePumpLibevent);
  MessagePumpLibevent::FileDescriptorWatcher* watcher =
      new MessagePumpLibevent::FileDescriptorWatcher;
  DeleteWatcher delegate(watcher);
  pump->WatchFileDescriptor(
      0, false, MessagePumpLibevent::WATCH_READ_WRITE, watcher, &delegate);

  // Spoof a libevent notification.
  OnLibeventNotification(pump, watcher);
}

class StopWatcher : public MessagePumpLibevent::Watcher {
 public:
  explicit StopWatcher(
      MessagePumpLibevent::FileDescriptorWatcher* controller)
      : controller_(controller) {
    DCHECK(controller_);
  }
  virtual ~StopWatcher() {}

  // base:MessagePumpLibevent::Watcher interface
  virtual void OnFileCanReadWithoutBlocking(int /* fd */) OVERRIDE {
    NOTREACHED();
  }
  virtual void OnFileCanWriteWithoutBlocking(int /* fd */) OVERRIDE {
    controller_->StopWatchingFileDescriptor();
  }

 private:
  MessagePumpLibevent::FileDescriptorWatcher* const controller_;
};

TEST_F(MessagePumpLibeventTest, StopWatcher) {
  scoped_refptr<MessagePumpLibevent> pump(new MessagePumpLibevent);
  MessagePumpLibevent::FileDescriptorWatcher watcher;
  StopWatcher delegate(&watcher);
  pump->WatchFileDescriptor(
      0, false, MessagePumpLibevent::WATCH_READ_WRITE, &watcher, &delegate);

  // Spoof a libevent notification.
  OnLibeventNotification(pump, &watcher);
}

}  // namespace

}  // namespace base
