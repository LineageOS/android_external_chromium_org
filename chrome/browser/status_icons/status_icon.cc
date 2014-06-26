// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/status_icons/status_icon.h"

#include "chrome/browser/status_icons/status_icon_observer.h"

StatusIcon::StatusIcon() {
}

StatusIcon::~StatusIcon() {
}

void StatusIcon::AddObserver(StatusIconObserver* observer) {
  observers_.AddObserver(observer);
}

void StatusIcon::RemoveObserver(StatusIconObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool StatusIcon::HasObservers() const {
  return observers_.might_have_observers();
}

void StatusIcon::DispatchClickEvent() {
  FOR_EACH_OBSERVER(StatusIconObserver, observers_, OnStatusIconClicked());
}

#if defined(OS_WIN)
void StatusIcon::DispatchBalloonClickEvent() {
  FOR_EACH_OBSERVER(StatusIconObserver, observers_, OnBalloonClicked());
}
#endif

void StatusIcon::ForceVisible() {}

void StatusIcon::SetContextMenu(scoped_ptr<StatusIconMenuModel> menu) {
  // The UI may been showing a menu for the current model, don't destroy it
  // until we've notified the UI of the change.
  scoped_ptr<StatusIconMenuModel> old_menu = context_menu_contents_.Pass();
  context_menu_contents_ = menu.Pass();
  UpdatePlatformContextMenu(context_menu_contents_.get());
}
