// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/pepper_truetype_font_list.h"

namespace content {

void GetFontFamilies_SlowBlocking(std::vector<std::string>* font_families) {
  NOTIMPLEMENTED();  // Font API isn't implemented on Android.
}

}  // namespace content
