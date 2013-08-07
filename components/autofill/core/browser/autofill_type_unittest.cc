// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace {

TEST(AutofillTypeTest, AutofillTypes) {
  // No server data.
  AutofillType none(NO_SERVER_DATA);
  EXPECT_EQ(NO_SERVER_DATA, none.server_type());
  EXPECT_EQ(NO_GROUP, none.group());

  // Unknown type.
  AutofillType unknown(UNKNOWN_TYPE);
  EXPECT_EQ(UNKNOWN_TYPE, unknown.server_type());
  EXPECT_EQ(NO_GROUP, unknown.group());

  // Type with group but no subgroup.
  AutofillType first(NAME_FIRST);
  EXPECT_EQ(NAME_FIRST, first.server_type());
  EXPECT_EQ(NAME, first.group());

  // Type with group and subgroup.
  AutofillType phone(PHONE_HOME_NUMBER);
  EXPECT_EQ(PHONE_HOME_NUMBER, phone.server_type());
  EXPECT_EQ(PHONE_HOME, phone.group());

  // Last value, to check any offset errors.
  AutofillType last(COMPANY_NAME);
  EXPECT_EQ(COMPANY_NAME, last.server_type());
  EXPECT_EQ(COMPANY, last.group());

  // Boundary (error) condition.
  AutofillType boundary(MAX_VALID_FIELD_TYPE);
  EXPECT_EQ(UNKNOWN_TYPE, boundary.server_type());
  EXPECT_EQ(NO_GROUP, boundary.group());

  // Beyond the boundary (error) condition.
  AutofillType beyond(static_cast<ServerFieldType>(MAX_VALID_FIELD_TYPE+10));
  EXPECT_EQ(UNKNOWN_TYPE, beyond.server_type());
  EXPECT_EQ(NO_GROUP, beyond.group());

  // In-between value.  Missing from enum but within range.  Error condition.
  AutofillType between(static_cast<ServerFieldType>(16));
  EXPECT_EQ(UNKNOWN_TYPE, between.server_type());
  EXPECT_EQ(NO_GROUP, between.group());
}

}  // namespace
}  // namespace autofill
