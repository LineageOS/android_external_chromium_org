// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_CONTEXT_MENU_H_
#define CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_CONTEXT_MENU_H_

#include "base/memory/scoped_ptr.h"

namespace aura {
class Window;
}

namespace ui {
class MenuModel;
}

// The multi user context menu factory.
scoped_ptr<ui::MenuModel> CreateMultiUserContextMenu(aura::Window* window);

#endif  // CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_CONTEXT_MENU_H_
