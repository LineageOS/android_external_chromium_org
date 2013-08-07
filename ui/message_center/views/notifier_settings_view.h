// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_NOTIFIER_SETTINGS_VIEW_H_
#define UI_MESSAGE_CENTER_VIEWS_NOTIFIER_SETTINGS_VIEW_H_

#include <set>

#include "base/memory/scoped_ptr.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/notifier_settings.h"
#include "ui/message_center/views/message_bubble_base.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/view.h"

namespace views {
class MenuRunner;
}

namespace message_center {
class NotifierGroupMenuModel;

// A class to show the list of notifier extensions / URL patterns and allow
// users to customize the settings.
class MESSAGE_CENTER_EXPORT NotifierSettingsView
    : public NotifierSettingsObserver,
      public views::View,
      public views::ButtonListener,
      public views::MenuButtonListener {
 public:
  explicit NotifierSettingsView(NotifierSettingsProvider* provider);
  virtual ~NotifierSettingsView();

  bool IsScrollable();

  // Overridden from NotifierSettingsDelegate:
  virtual void UpdateIconImage(const NotifierId& notifier_id,
                               const gfx::Image& icon) OVERRIDE;
  virtual void NotifierGroupChanged() OVERRIDE;

  void set_provider(NotifierSettingsProvider* new_provider) {
    provider_ = new_provider;
  }

 private:
  class NotifierButton;

  // Given a new list of notifiers, updates the view to reflect it.
  void UpdateContentsView(const std::vector<Notifier*>& notifiers);

  // Overridden from views::View:
  virtual void Layout() OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual bool OnKeyPressed(const ui::KeyEvent& event) OVERRIDE;
  virtual bool OnMouseWheel(const ui::MouseWheelEvent& event) OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;
  virtual void OnMenuButtonClicked(views::View* source,
                                   const gfx::Point& point) OVERRIDE;

  views::ImageButton* title_arrow_;
  views::View* title_entry_;
  views::ScrollView* scroller_;
  NotifierSettingsProvider* provider_;
  std::set<NotifierButton*> buttons_;
  scoped_ptr<NotifierGroupMenuModel> notifier_group_menu_model_;
  scoped_ptr<views::MenuRunner> notifier_group_menu_runner_;

  DISALLOW_COPY_AND_ASSIGN(NotifierSettingsView);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_NOTIFIER_SETTINGS_VIEW_H_
