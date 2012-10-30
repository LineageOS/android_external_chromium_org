// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/chromeos/keyboard_handler.h"

#include "base/values.h"
#include "chrome/browser/chromeos/input_method/xkeyboard.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
const struct ModifierKeysSelectItem {
  int message_id;
  chromeos::input_method::ModifierKey value;
} kModifierKeysSelectItems[] = {
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_SEARCH,
    chromeos::input_method::kSearchKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_CTRL,
    chromeos::input_method::kControlKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_ALT,
    chromeos::input_method::kAltKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_VOID,
    chromeos::input_method::kVoidKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_CAPS_LOCK,
    chromeos::input_method::kCapsLockKey },
};

const char* kDataValuesNames[] = {
  "remapSearchKeyToValue",
  "remapControlKeyToValue",
  "remapAltKeyToValue",
};
}  // namespace

namespace chromeos {
namespace options {

KeyboardHandler::KeyboardHandler() {
}

KeyboardHandler::~KeyboardHandler() {
}

void KeyboardHandler::GetLocalizedValues(DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  localized_strings->SetString("keyboardOverlayTitle",
      l10n_util::GetStringUTF16(IDS_OPTIONS_KEYBOARD_OVERLAY_TITLE));
  localized_strings->SetString("remapSearchKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_SEARCH_LABEL));
  localized_strings->SetString("remapControlKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_CTRL_LABEL));
  localized_strings->SetString("remapAltKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_ALT_LABEL));

  for (size_t i = 0; i < arraysize(kDataValuesNames); ++i) {
    ListValue* list_value = new ListValue();
    for (size_t j = 0; j < arraysize(kModifierKeysSelectItems); ++j) {
      const input_method::ModifierKey value =
          kModifierKeysSelectItems[j].value;
      const int message_id = kModifierKeysSelectItems[j].message_id;
      // Only the seach key can be remapped to the caps lock key.
      if (kDataValuesNames[i] != std::string("remapSearchKeyToValue") &&
          value == input_method::kCapsLockKey) {
        continue;
      }
      ListValue* option = new ListValue();
      option->Append(Value::CreateIntegerValue(value));
      option->Append(Value::CreateStringValue(l10n_util::GetStringUTF16(
          message_id)));
      list_value->Append(option);
    }
    localized_strings->Set(kDataValuesNames[i], list_value);
  }
}

}  // namespace options
}  // namespace chromeos
