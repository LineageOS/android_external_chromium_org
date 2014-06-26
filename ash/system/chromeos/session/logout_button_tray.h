// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_CHROMEOS_SESSION_LOGOUT_BUTTON_TRAY_H_
#define ASH_SYSTEM_CHROMEOS_SESSION_LOGOUT_BUTTON_TRAY_H_

#include "ash/ash_export.h"
#include "ash/system/chromeos/session/logout_button_observer.h"
#include "ash/system/tray/tray_background_view.h"
#include "ash/system/user/login_status.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/time/time.h"
#include "ui/views/controls/button/button.h"

namespace views {
class LabelButton;
}

namespace ash {
class StatusAreaWidget;

// Adds a logout button to the launcher's status area if enabled by the
// kShowLogoutButtonInTray pref.
class ASH_EXPORT LogoutButtonTray : public TrayBackgroundView,
                                    public LogoutButtonObserver,
                                    public views::ButtonListener {
 public:
  explicit LogoutButtonTray(StatusAreaWidget* status_area_widget);
  virtual ~LogoutButtonTray();

  // TrayBackgroundView:
  virtual void SetShelfAlignment(ShelfAlignment alignment) OVERRIDE;
  virtual base::string16 GetAccessibleNameForTray() OVERRIDE;
  virtual void HideBubbleWithView(
      const views::TrayBubbleView* bubble_view) OVERRIDE;
  virtual bool ClickedOutsideBubble() OVERRIDE;

  // LogoutButtonObserver:
  virtual void OnShowLogoutButtonInTrayChanged(bool show) OVERRIDE;
  virtual void OnLogoutDialogDurationChanged(base::TimeDelta duration) OVERRIDE;

  // views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  void UpdateAfterLoginStatusChange(user::LoginStatus login_status);

 private:
  void UpdateVisibility();

  views::LabelButton* button_;
  user::LoginStatus login_status_;
  bool show_logout_button_in_tray_;
  base::TimeDelta dialog_duration_;

  DISALLOW_COPY_AND_ASSIGN(LogoutButtonTray);
};

}  // namespace ash

#endif  // ASH_SYSTEM_CHROMEOS_SESSION_LOGOUT_BUTTON_TRAY_H_
