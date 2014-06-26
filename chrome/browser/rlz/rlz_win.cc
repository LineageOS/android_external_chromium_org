// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/rlz/rlz.h"

// static
rlz_lib::AccessPoint RLZTracker::ChromeOmnibox() {
  return rlz_lib::CHROME_OMNIBOX;
}

// static
rlz_lib::AccessPoint RLZTracker::ChromeHomePage() {
  return rlz_lib::CHROME_HOME_PAGE;
}

// static
rlz_lib::AccessPoint RLZTracker::ChromeAppList() {
  return rlz_lib::CHROME_APP_LIST;
}
