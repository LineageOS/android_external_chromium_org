// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>
#include <stdlib.h>

#include "sandbox/linux/tests/unit_tests.h"

namespace sandbox {

namespace {

// Let's not use any of the "magic" values used internally in unit_tests.cc,
// such as kExpectedValue.
const int kExpectedExitCode = 100;

SANDBOX_DEATH_TEST(UnitTests,
                   DeathExitCode,
                   DEATH_EXIT_CODE(kExpectedExitCode)) {
  exit(kExpectedExitCode);
}

const int kExpectedSignalNumber = SIGKILL;

SANDBOX_DEATH_TEST(UnitTests,
                   DeathBySignal,
                   DEATH_BY_SIGNAL(kExpectedSignalNumber)) {
  raise(kExpectedSignalNumber);
}

}  // namespace

}  // namespace sandbox
