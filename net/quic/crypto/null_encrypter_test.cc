// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/crypto/null_encrypter.h"
#include "net/quic/test_tools/quic_test_utils.h"

using base::StringPiece;

namespace net {
namespace test {

TEST(NullEncrypterTest, Encrypt) {
  unsigned char expected[] = {
    // fnv hash
    0xa0, 0x6f, 0x44, 0x8a,
    0x44, 0xf8, 0x18, 0x3b,
    0x47, 0x91, 0xb2, 0x13,
    0x6b, 0x09, 0xbb, 0xae,
    // payload
    'g',  'o',  'o',  'd',
    'b',  'y',  'e',  '!',
  };
  NullEncrypter encrypter;
  scoped_ptr<QuicData> encrypted(encrypter.Encrypt(0, "hello world!",
                                                   "goodbye!"));
  ASSERT_TRUE(encrypted.get());
  test::CompareCharArraysWithHexError(
      "encrypted data", encrypted->data(), encrypted->length(),
      reinterpret_cast<const char*>(expected), arraysize(expected));
}

TEST(NullEncrypterTest, GetMaxPlaintextSize) {
  NullEncrypter encrypter;
  EXPECT_EQ(1000u, encrypter.GetMaxPlaintextSize(1016));
  EXPECT_EQ(100u, encrypter.GetMaxPlaintextSize(116));
  EXPECT_EQ(10u, encrypter.GetMaxPlaintextSize(26));
}

TEST(NullEncrypterTest, GetCiphertextSize) {
  NullEncrypter encrypter;
  EXPECT_EQ(1016u, encrypter.GetCiphertextSize(1000));
  EXPECT_EQ(116u, encrypter.GetCiphertextSize(100));
  EXPECT_EQ(26u, encrypter.GetCiphertextSize(10));
}

}  // namespace test
}  // namespace net
