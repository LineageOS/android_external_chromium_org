// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_KEY_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_KEY_H_

#include <string>

namespace chromeos {

// Key for user authentication. The class supports hashing of plain text
// passwords to generate keys as well as the use of pre-hashed keys.
class Key {
 public:
  enum KeyType {
    // Plain text password.
    KEY_TYPE_PASSWORD_PLAIN,
    // SHA256 of salt + password, first half only, lower-case hex encoded.
    KEY_TYPE_SALTED_SHA256_TOP_HALF,
    // PBKDF2 with 256 bit AES and 1234 iterations, base64 encoded.
    KEY_TYPE_SALTED_PBKDF2_AES256_1234,
  };

  Key();
  Key(const Key& other);
  explicit Key(const std::string& plain_text_password);
  Key(KeyType key_type, const std::string& salt, const std::string& secret);
  ~Key();

  bool operator==(const Key& other) const;

  KeyType GetKeyType() const;
  const std::string& GetSecret() const;
  const std::string& GetLabel() const;

  void SetLabel(const std::string& label);

  void ClearSecret();

  void Transform(KeyType target_key_type, const std::string& salt);

 private:
  KeyType key_type_;
  std::string salt_;
  std::string secret_;
  std::string label_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_KEY_H_
