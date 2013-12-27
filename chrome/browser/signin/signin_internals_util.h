// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_SIGNIN_INTERNALS_UTIL_H_
#define CHROME_BROWSER_SIGNIN_SIGNIN_INTERNALS_UTIL_H_

#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/values.h"

namespace signin_internals_util {

// Preference prefixes for signin and token values.
extern const char kSigninPrefPrefix[];
extern const char kTokenPrefPrefix[];

// The length of strings returned by GetTruncatedHash() below.
const size_t kTruncateTokenStringLength = 6;

// Helper enums to access fields from SigninStatus (declared below).
enum {
  SIGNIN_FIELDS_BEGIN = 0,
  UNTIMED_FIELDS_BEGIN = SIGNIN_FIELDS_BEGIN
};

enum UntimedSigninStatusField {
  USERNAME = UNTIMED_FIELDS_BEGIN,
  UNTIMED_FIELDS_END
};

enum {
  UNTIMED_FIELDS_COUNT = UNTIMED_FIELDS_END - UNTIMED_FIELDS_BEGIN,
  TIMED_FIELDS_BEGIN = UNTIMED_FIELDS_END
};

enum TimedSigninStatusField {
  SIGNIN_TYPE = TIMED_FIELDS_BEGIN,
  CLIENT_LOGIN_STATUS,
  OAUTH_LOGIN_STATUS,
  GET_USER_INFO_STATUS,
  UBER_TOKEN_STATUS,
  MERGE_SESSION_STATUS,
  TIMED_FIELDS_END
};

enum {
  TIMED_FIELDS_COUNT = TIMED_FIELDS_END - TIMED_FIELDS_BEGIN,
  SIGNIN_FIELDS_END = TIMED_FIELDS_END,
  SIGNIN_FIELDS_COUNT = SIGNIN_FIELDS_END - SIGNIN_FIELDS_BEGIN
};

// Returns the root preference path for the service. The path should be
// qualified with one of .value, .status or .time to get the respective
// full preference path names.
std::string TokenPrefPath(const std::string& service_name);

// Returns the name of a SigninStatus field.
std::string SigninStatusFieldToString(UntimedSigninStatusField field);
std::string SigninStatusFieldToString(TimedSigninStatusField field);

// An Observer class for authentication and token diagnostic information.
class SigninDiagnosticsObserver {
 public:
  // Credentials and signin related changes.
  virtual void NotifySigninValueChanged(const UntimedSigninStatusField& field,
                                        const std::string& value) {}
  virtual void NotifySigninValueChanged(const TimedSigninStatusField& field,
                                        const std::string& value) {}
  // OAuth tokens related changes.
  virtual void NotifyTokenReceivedSuccess(const std::string& token_name,
                                          const std::string& token,
                                          bool update_time) {}
  virtual void NotifyTokenReceivedFailure(const std::string& token_name,
                                          const std::string& error) {}
  virtual void NotifyClearStoredToken(const std::string& token_name) {}};

// Gets the first 6 hex characters of the SHA256 hash of the passed in string.
// These are enough to perform equality checks across a single users tokens,
// while preventing outsiders from reverse-engineering the actual token from
// the displayed value.
// Note that for readability (in about:signin-internals), an empty string
// is not hashed, but simply returned as an empty string.
std::string GetTruncatedHash(const std::string& str);

} // namespace signin_internals_util

#endif  // CHROME_BROWSER_SIGNIN_SIGNIN_INTERNALS_UTIL_H_
