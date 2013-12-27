// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_internals_util.h"

#include <sstream>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "google_apis/gaia/gaia_constants.h"

namespace signin_internals_util {

const char kSigninPrefPrefix[] = "google.services.signin.";
const char kTokenPrefPrefix[] = "google.services.signin.tokens.";

#define ENUM_CASE(x) case x: return (std::string(kSigninPrefPrefix) + #x)
std::string SigninStatusFieldToString(UntimedSigninStatusField field) {
  switch (field) {
    ENUM_CASE(USERNAME);
    case UNTIMED_FIELDS_END:
      NOTREACHED();
      return std::string();
  }

  NOTREACHED();
  return std::string();
}

std::string SigninStatusFieldToString(TimedSigninStatusField field) {
  switch (field) {
    ENUM_CASE(SIGNIN_TYPE);
    ENUM_CASE(CLIENT_LOGIN_STATUS);
    ENUM_CASE(OAUTH_LOGIN_STATUS);
    ENUM_CASE(GET_USER_INFO_STATUS);
    ENUM_CASE(UBER_TOKEN_STATUS);
    ENUM_CASE(MERGE_SESSION_STATUS);
    case TIMED_FIELDS_END:
      NOTREACHED();
      return std::string();
  }

  NOTREACHED();
  return std::string();
}

std::string TokenPrefPath(const std::string& token_name) {
  return std::string(kTokenPrefPrefix) + token_name;
}

// Gets the first few hex characters of the SHA256 hash of the passed in string.
// These are enough to perform equality checks across a single users tokens,
// while preventing outsiders from reverse-engineering the actual token from
// the displayed value.
// Note that for readability (in about:signin-internals), an empty string
// is not hashed, but simply returned as an empty string.
std::string GetTruncatedHash(const std::string& str) {
  if (str.empty())
    return str;

  // Since each character in the hash string generates two hex charaters
  // we only need half as many charaters in |hash_val| as hex characters
  // returned.
  const int kTruncateSize = kTruncateTokenStringLength / 2;
  char hash_val[kTruncateSize];
  crypto::SHA256HashString(str, &hash_val[0], kTruncateSize);
  return StringToLowerASCII(base::HexEncode(&hash_val[0], kTruncateSize));
}

} //  namespace signin_internals_util
