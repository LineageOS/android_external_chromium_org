// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_view.h"

#include <sstream>

#include "ppapi/c/pp_time.h"
#include "ppapi/c/dev/ppb_testing_dev.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(View);

// When waiting for view changed events, wait no longer than this.
static int kViewChangeTimeoutSec = 5;

TestView::TestView(TestingInstance* instance)
    : TestCase(instance),
      post_quit_on_view_changed_(false) {
}

void TestView::DidChangeView(const pp::View& view) {
  last_view_ = view;
  page_visibility_log_.push_back(view.IsPageVisible());

  if (post_quit_on_view_changed_) {
    post_quit_on_view_changed_ = false;
    testing_interface_->QuitMessageLoop(instance_->pp_instance());
  }
}

bool TestView::Init() {
  return CheckTestingInterface();
}

void TestView::RunTests(const std::string& filter) {
  RUN_TEST(CreatedVisible, filter);
  RUN_TEST(CreatedInvisible, filter);
  RUN_TEST(PageHideShow, filter);
  RUN_TEST(SizeChange, filter);
  RUN_TEST(ClipChange, filter);
}

bool TestView::WaitUntilViewChanged() {
  // Schedule a callback so this step times out if we don't get a ViewChanged
  // in a reasonable amount of time.
  pp::CompletionCallbackFactory<TestView> factory(this);
  pp::CompletionCallback timeout =
      factory.NewCallback(&TestView::QuitMessageLoop);
  pp::Module::Get()->core()->CallOnMainThread(
      kViewChangeTimeoutSec * 1000, timeout);

  size_t old_page_visibility_change_count = page_visibility_log_.size();

  // Run a nested message loop. It will exit either on ViewChanged or if the
  // timeout happens.
  post_quit_on_view_changed_ = true;
  testing_interface_->RunMessageLoop(instance_->pp_instance());
  post_quit_on_view_changed_ = false;

  // We know we got a view changed event if something was appended to the log.
  return page_visibility_log_.size() > old_page_visibility_change_count;
}

void TestView::QuitMessageLoop(int32_t result) {
  testing_interface_->QuitMessageLoop(instance_->pp_instance());
}

std::string TestView::TestCreatedVisible() {
  ASSERT_FALSE(page_visibility_log_.empty());
  ASSERT_TRUE(page_visibility_log_[0]);
  PASS();
}

std::string TestView::TestCreatedInvisible() {
  ASSERT_FALSE(page_visibility_log_.empty());

  if (page_visibility_log_[0]) {
    // Add more error message since this test has some extra requirements.
    instance_->AppendError("Initial page is set to visible. NOTE: "
        "This test must be run in a background tab. "
        "Either run in the UI test which does this, or you can middle-click "
        "on the test link to run manually.");
  }
  ASSERT_FALSE(page_visibility_log_[0]);
  PASS();
}

std::string TestView::TestPageHideShow() {
  // Initial state should be visible.
  ASSERT_FALSE(page_visibility_log_.empty());
  ASSERT_TRUE(page_visibility_log_[0]);

  // Now that we're alive, set a cookie so the UI test knows it can change our
  // visibility.
  instance_->SetCookie("TestPageHideShow:Created", "TRUE");

  // Wait until we get a hide event, being careful to handle spurious
  // notifications of ViewChanged.
  PP_Time begin_time = pp::Module::Get()->core()->GetTime();
  while (WaitUntilViewChanged() &&
         page_visibility_log_[page_visibility_log_.size() - 1] &&
         pp::Module::Get()->core()->GetTime() - begin_time <
             kViewChangeTimeoutSec) {
  }
  if (page_visibility_log_[page_visibility_log_.size() - 1]) {
    // Didn't get a view changed event that changed visibility (though there
    // may have been some that didn't change visibility).
    // Add more error message since this test has some extra requirements.
    return "Didn't receive a hide event in timeout. NOTE: "
        "This test requires tab visibility to change and won't pass if you "
        "just run it in a browser. Normally the UI test should handle "
        "this. You can also run manually by waiting 2 secs, creating a new "
        "tab, waiting 2 more secs, and closing the new tab.";
  }

  // Set a cookie so the UI test knows it can show us again.
  instance_->SetCookie("TestPageHideShow:Hidden", "TRUE");

  // Wait until we get a show event.
  begin_time = pp::Module::Get()->core()->GetTime();
  while (WaitUntilViewChanged() &&
         !page_visibility_log_[page_visibility_log_.size() - 1] &&
         pp::Module::Get()->core()->GetTime() - begin_time <
             kViewChangeTimeoutSec) {
  }
  ASSERT_TRUE(page_visibility_log_[page_visibility_log_.size() - 1]);

  PASS();
}

std::string TestView::TestSizeChange() {
  pp::Rect original_rect = last_view_.GetRect();

  pp::Rect desired_rect = original_rect;
  desired_rect.set_width(original_rect.width() + 10);
  desired_rect.set_height(original_rect.height() + 12);

  std::ostringstream script_stream;
  script_stream << "var plugin = document.getElementById('plugin');";
  script_stream << "plugin.setAttribute('width', "
                << desired_rect.width() << ");";
  script_stream << "plugin.setAttribute('height', "
                << desired_rect.height() << ");";

  instance_->EvalScript(script_stream.str());

  PP_Time begin_time = pp::Module::Get()->core()->GetTime();
  while (WaitUntilViewChanged() && last_view_.GetRect() != desired_rect &&
         pp::Module::Get()->core()->GetTime() - begin_time <
             kViewChangeTimeoutSec) {
  }
  ASSERT_TRUE(last_view_.GetRect() == desired_rect);

  PASS();
}

std::string TestView::TestClipChange() {
  pp::Rect original_rect = last_view_.GetRect();

  // Original clip should be the full frame.
  pp::Rect original_clip = last_view_.GetClipRect();
  ASSERT_TRUE(original_clip.x() == 0);
  ASSERT_TRUE(original_clip.y() == 0);
  ASSERT_TRUE(original_clip.width() == original_rect.width());
  ASSERT_TRUE(original_clip.height() == original_rect.height());

  int clip_amount = original_rect.height() / 2;

  // It might be nice to set the position to be absolute and set the location,
  // but this will cause WebKit to actually tear down the plugin and recreate
  // it. So instead we add a big div to cause the document to be scrollable,
  // and scroll it down.
  std::ostringstream script_stream;
  script_stream
      << "var big = document.createElement('div');"
      << "big.setAttribute('style', 'position:absolute; left:100px; "
                                    "top:0px; width:1px; height:5000px;');"
      << "document.body.appendChild(big);"
      << "window.scrollBy(0, " << original_rect.y() + clip_amount << ");";

  instance_->EvalScript(script_stream.str());

  pp::Rect desired_clip = original_clip;
  desired_clip.set_y(clip_amount);
  desired_clip.set_height(desired_clip.height() - desired_clip.y());

  PP_Time begin_time = pp::Module::Get()->core()->GetTime();
  while (WaitUntilViewChanged() && last_view_.GetClipRect() != desired_clip &&
         pp::Module::Get()->core()->GetTime() - begin_time <
             kViewChangeTimeoutSec) {
  }
  ASSERT_TRUE(last_view_.GetClipRect() == desired_clip);
  PASS();
}
