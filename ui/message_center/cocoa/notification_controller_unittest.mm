// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/notification_controller.h"

#include "base/mac/foundation_util.h"
#include "base/memory/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "base/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#import "ui/base/cocoa/hover_image_button.h"
#import "ui/base/test/ui_cocoa_test_helper.h"
#include "ui/message_center/fake_message_center.h"
#include "ui/message_center/message_center_constants.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_types.h"

namespace {

class MockMessageCenter : public message_center::FakeMessageCenter {
 public:
  MockMessageCenter()
      : last_removed_by_user_(false),
        remove_count_(0),
        last_clicked_index_(-1) {}

  virtual void RemoveNotification(const std::string& id,
                                  bool by_user) OVERRIDE {
    last_removed_id_ = id;
    last_removed_by_user_ = by_user;
    ++remove_count_;
  }

  virtual void ClickOnNotificationButton(const std::string& id,
                                         int button_index) OVERRIDE {
    last_clicked_id_ = id;
    last_clicked_index_ = button_index;
  }

  const std::string& last_removed_id() const { return last_removed_id_; }
  bool last_removed_by_user() const { return last_removed_by_user_; }
  int remove_count() const { return remove_count_; }
  const std::string& last_clicked_id() const { return last_clicked_id_; }
  int last_clicked_index() const { return last_clicked_index_; }

 private:
  std::string last_removed_id_;
  bool last_removed_by_user_;
  int remove_count_;

  std::string last_clicked_id_;
  int last_clicked_index_;

  DISALLOW_COPY_AND_ASSIGN(MockMessageCenter);
};

}

@implementation MCNotificationController (TestingInterface)
- (NSButton*)closeButton {
  return closeButton_.get();
}

- (NSButton*)secondButton {
  // The buttons are in Cocoa-y-order, so the 2nd button is first.
  NSView* view = [[bottomView_ subviews] objectAtIndex:0];
  return base::mac::ObjCCastStrict<NSButton>(view);
}

- (NSImageView*)iconView {
  return icon_.get();
}

- (NSTextField*)titleView {
  return title_.get();
}

- (NSTextField*)messageView {
  return message_.get();
}
@end

class NotificationControllerTest : public ui::CocoaTest {
 public:
  NSImage* TestIcon() {
    return [NSImage imageNamed:NSImageNameUser];
  }
};

TEST_F(NotificationControllerTest, BasicLayout) {
  scoped_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE,
          "",
          ASCIIToUTF16("Added to circles"),
          ASCIIToUTF16("Jonathan and 5 others"),
          string16(),
          std::string(),
          NULL));
  notification->set_icon(gfx::Image([TestIcon() retain]));

  scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);
  [controller view];

  EXPECT_EQ(TestIcon(), [[controller iconView] image]);
  EXPECT_EQ(base::SysNSStringToUTF16([[controller titleView] stringValue]),
            notification->title());
  EXPECT_EQ(base::SysNSStringToUTF16([[controller messageView] stringValue]),
            notification->message());
  EXPECT_EQ(controller.get(), [[controller closeButton] target]);
}

TEST_F(NotificationControllerTest, OverflowText) {
  scoped_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE,
          "",
          ASCIIToUTF16("This is a much longer title that should wrap "
                       "multiple lines."),
          ASCIIToUTF16("And even the message is long. This sure is a wordy "
                       "notification. Are you really going to read this "
                       "entire thing?"),
          string16(),
          std::string(),
          NULL));
  scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);
  [controller view];

  EXPECT_GT(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
}

TEST_F(NotificationControllerTest, Close) {
  scoped_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE,
          "an_id",
          string16(),
          string16(),
          string16(),
          std::string(),
          NULL));
  MockMessageCenter message_center;

  scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  [[controller closeButton] performClick:nil];

  EXPECT_EQ(1, message_center.remove_count());
  EXPECT_EQ("an_id", message_center.last_removed_id());
  EXPECT_TRUE(message_center.last_removed_by_user());
}

TEST_F(NotificationControllerTest, Update) {
  scoped_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE,
          "",
          ASCIIToUTF16("A simple title"),
          ASCIIToUTF16("This message isn't too long and should fit in the"
                       "default bounds."),
          string16(),
          std::string(),
          NULL));
  scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);

  // Set up the default layout.
  [controller view];
  EXPECT_EQ(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
  EXPECT_FALSE([[controller iconView] image]);

  // Update the icon.
  notification->set_icon(gfx::Image([TestIcon() retain]));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(TestIcon(), [[controller iconView] image]);
  EXPECT_EQ(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
}

TEST_F(NotificationControllerTest, Buttons) {
  base::DictionaryValue buttons;
  buttons.SetString(message_center::kButtonOneTitleKey, "button1");
  buttons.SetString(message_center::kButtonTwoTitleKey, "button2");

  scoped_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_BASE_FORMAT,
          "an_id",
          string16(),
          string16(),
          string16(),
          std::string(),
          &buttons));
  MockMessageCenter message_center;

  scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  [[controller secondButton] performClick:nil];

  EXPECT_EQ("an_id", message_center.last_clicked_id());
  EXPECT_EQ(1, message_center.last_clicked_index());
}
