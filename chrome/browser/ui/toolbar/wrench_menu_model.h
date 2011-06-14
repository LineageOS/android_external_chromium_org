// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_WRENCH_MENU_MODEL_H_
#define CHROME_BROWSER_UI_TOOLBAR_WRENCH_MENU_MODEL_H_
#pragma once

#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/tabs/tab_strip_model_observer.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "ui/base/models/accelerator.h"
#include "ui/base/models/button_menu_item_model.h"
#include "ui/base/models/simple_menu_model.h"

class Browser;
class TabStripModel;

namespace {
class MockWrenchMenuModel;
}  // namespace

// A menu model that builds the contents of an encoding menu.
class EncodingMenuModel : public ui::SimpleMenuModel,
                          public ui::SimpleMenuModel::Delegate {
 public:
  explicit EncodingMenuModel(Browser* browser);
  virtual ~EncodingMenuModel();

  // Overridden from ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;
  virtual void ExecuteCommand(int command_id) OVERRIDE;

 private:
  void Build();

  Browser* browser_;  // weak

  DISALLOW_COPY_AND_ASSIGN(EncodingMenuModel);
};

// A menu model that builds the contents of the zoom menu.
class ZoomMenuModel : public ui::SimpleMenuModel {
 public:
  explicit ZoomMenuModel(ui::SimpleMenuModel::Delegate* delegate);
  virtual ~ZoomMenuModel();

 private:
  void Build();

  DISALLOW_COPY_AND_ASSIGN(ZoomMenuModel);
};

class ToolsMenuModel : public ui::SimpleMenuModel {
 public:
  ToolsMenuModel(ui::SimpleMenuModel::Delegate* delegate, Browser* browser);
  virtual ~ToolsMenuModel();

 private:
  void Build(Browser* browser);

  scoped_ptr<EncodingMenuModel> encoding_menu_model_;

  DISALLOW_COPY_AND_ASSIGN(ToolsMenuModel);
};

class BookmarkSubMenuModel : public ui::SimpleMenuModel {
 public:
  BookmarkSubMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                       Browser* browser);
  virtual ~BookmarkSubMenuModel();

 private:
  void Build(Browser* browser);

  DISALLOW_COPY_AND_ASSIGN(BookmarkSubMenuModel);
};

class ProfilesSubMenuModel : public ui::SimpleMenuModel,
                             public ui::SimpleMenuModel::Delegate {
 public:
  ProfilesSubMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                       Browser* browser);

  virtual void ExecuteCommand(int command_id) OVERRIDE;
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;

 private:
  enum {
    // The profiles submenu contains a menu item for each profile. For
    // the i'th profile the command ID is COMMAND_SWITCH_TO_PROFILE + i.
    // If the profile matches the profile of the wrench button's browser
    // then the menu item is checked.
    COMMAND_SWITCH_TO_PROFILE,
  };

  void Build();

  Browser* browser_;  // weak
  ui::SimpleMenuModel::Delegate* delegate_;  // weak

  DISALLOW_COPY_AND_ASSIGN(ProfilesSubMenuModel);
};

// A menu model that builds the contents of the wrench menu.
class WrenchMenuModel : public ui::SimpleMenuModel,
                        public ui::SimpleMenuModel::Delegate,
                        public ui::ButtonMenuItemModel::Delegate,
                        public TabStripModelObserver,
                        public NotificationObserver {
 public:
  WrenchMenuModel(ui::AcceleratorProvider* provider, Browser* browser);
  virtual ~WrenchMenuModel();

  // Overridden for ButtonMenuItemModel::Delegate:
  virtual bool DoesCommandIdDismissMenu(int command_id) const OVERRIDE;

  // Overridden for both ButtonMenuItemModel::Delegate and SimpleMenuModel:
  virtual bool IsItemForCommandIdDynamic(int command_id) const OVERRIDE;
  virtual string16 GetLabelForCommandId(int command_id) const OVERRIDE;
  virtual bool GetIconForCommandId(int command_id,
                                   SkBitmap* icon) const OVERRIDE;
  virtual void ExecuteCommand(int command_id) OVERRIDE;
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool IsCommandIdVisible(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;

  // Overridden from TabStripModelObserver:
  virtual void ActiveTabChanged(TabContentsWrapper* old_contents,
                                TabContentsWrapper* new_contents,
                                int index,
                                bool user_gesture) OVERRIDE;
  virtual void TabReplacedAt(TabStripModel* tab_strip_model,
                             TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index) OVERRIDE;
  virtual void TabStripModelDeleted() OVERRIDE;

  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) OVERRIDE;

  // Getters.
  Browser* browser() const { return browser_; }

  // Calculates |zoom_label_| in response to a zoom change.
  void UpdateZoomControls();

 private:
  // Testing constructor used for mocking.
  friend class ::MockWrenchMenuModel;
  WrenchMenuModel();

  void Build();

  // Adds custom items to the menu. Deprecated in favor of a cross platform
  // model for button items.
  void CreateCutCopyPaste();
  void CreateZoomFullscreen();

  string16 GetSyncMenuLabel() const;

  // Models for the special menu items with buttons.
  scoped_ptr<ui::ButtonMenuItemModel> edit_menu_item_model_;
  scoped_ptr<ui::ButtonMenuItemModel> zoom_menu_item_model_;

  // Label of the zoom label in the zoom menu item.
  string16 zoom_label_;

  // Tools menu.
  scoped_ptr<ToolsMenuModel> tools_menu_model_;

  // Bookmark submenu.
  scoped_ptr<BookmarkSubMenuModel> bookmark_sub_menu_model_;

  // Profiles submenu.
  scoped_ptr<ProfilesSubMenuModel> profiles_sub_menu_model_;

  ui::AcceleratorProvider* provider_;  // weak

  Browser* browser_;  // weak
  TabStripModel* tabstrip_model_; // weak

  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(WrenchMenuModel);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_WRENCH_MENU_MODEL_H_
