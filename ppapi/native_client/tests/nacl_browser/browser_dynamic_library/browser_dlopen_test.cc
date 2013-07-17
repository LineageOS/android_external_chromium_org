/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/native_client/tests/ppapi_test_lib/get_browser_interface.h"
#include "ppapi/native_client/tests/ppapi_test_lib/test_interface.h"
#include "ppapi/native_client/tests/ppapi_test_lib/testable_callback.h"


namespace {

void TestDlopenMainThread() {
  // This is a valid .so to load up, but dlopen doesn't work from the main
  // PPAPI thread, so it should always fail.
  void* lib_handle = dlopen("libmemusage.so", RTLD_LAZY);
  EXPECT(lib_handle == NULL);
  TEST_PASSED;
}


void CheckSecondaryThreadSuccess(void *lib_handle, int32_t unused_result) {
  EXPECT(lib_handle != NULL);
  PostTestMessage("TestDlopenSecondaryThread", "PASSED");
}

void* SecondaryThreadFunc(void *unused_data) {
  void* lib_handle = dlopen("libmemusage.so", RTLD_LAZY);
  PP_CompletionCallback callback = PP_MakeCompletionCallback(
      CheckSecondaryThreadSuccess,
      lib_handle);
  PPBCore()->CallOnMainThread(0, callback, PP_OK);
  return NULL;
}

void TestDlopenSecondaryThread() {
  pthread_t p;
  pthread_create(&p, NULL, SecondaryThreadFunc, NULL);
  // This function must return in order for the main message loop to
  // service the requests issued from the dlopen call, we can't wait
  // for the result of the thread here.  The 'PASSED' message will
  // be generated by the thread.
}
}

void SetupTests() {
  RegisterTest("TestDlopenMainThread", TestDlopenMainThread);
  RegisterTest("TestDlopenSecondaryThread", TestDlopenSecondaryThread);
}

void SetupPluginInterfaces() {
  // none
}
