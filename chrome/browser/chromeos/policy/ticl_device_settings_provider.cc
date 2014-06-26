// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/ticl_device_settings_provider.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"

namespace policy {

TiclDeviceSettingsProvider::TiclDeviceSettingsProvider() {
}

TiclDeviceSettingsProvider::~TiclDeviceSettingsProvider() {
}

bool TiclDeviceSettingsProvider::UseGCMChannel() const {
  return CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kInvalidationUseGCMChannel);
}

}  // namespace policy
