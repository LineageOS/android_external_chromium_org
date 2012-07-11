// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains helper functions for getting strings that are included in
// our DLL for all languages (i.e., does not come from our language DLL).
//
// These resource strings are organized such that we can get a localized string
// by taking the base resource ID and adding a language offset.  For example,
// to get the resource id for the localized product name in en-US, we take
// IDS_PRODUCT_NAME_BASE + IDS_L10N_OFFSET_EN_US.

#ifndef CHROME_INSTALLER_UTIL_L10N_STRING_UTIL_H_
#define CHROME_INSTALLER_UTIL_L10N_STRING_UTIL_H_

#include <string>

#include "base/string16.h"

namespace installer {

class TranslationDelegate {
 public:
  virtual ~TranslationDelegate();
  virtual string16 GetLocalizedString(int installer_string_id) = 0;
};

// If we're in Chrome, the installer strings aren't in the binary, but are in
// the localized pak files.  A TranslationDelegate must be provided so we can
// load these strings.
void SetTranslationDelegate(TranslationDelegate* delegate);

// Given a string base id, return the localized version of the string based on
// the system language.  This is used for shortcuts placed on the user's
// desktop.  The string is retrieved from the TranslationDelegate if one has
// been set.  Otherwise, the string is read from the binary's string table.
std::wstring GetLocalizedString(int base_message_id);

// Given the system language, return a url that points to the localized eula.
// The empty string is returned on failure.
std::wstring GetLocalizedEulaResource();

// Returns the language identifier of the translation currently in use.
std::wstring GetCurrentTranslation();

}  // namespace installer.

#endif  // CHROME_INSTALLER_UTIL_L10N_STRING_UTIL_H_
