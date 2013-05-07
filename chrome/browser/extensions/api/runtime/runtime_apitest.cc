// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/runtime/runtime_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"

// Tests the privileged components of chrome.runtime.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimePrivileged) {
  ASSERT_TRUE(RunExtensionTest("runtime/privileged")) << message_;
}

// Tests the unprivileged components of chrome.runtime.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeUnprivileged) {
  ASSERT_TRUE(StartTestServer());
  ASSERT_TRUE(
      LoadExtension(test_data_dir_.AppendASCII("runtime/content_script")));

  // The content script runs on webpage.html.
  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               test_server()->GetURL("webpage.html"));
  EXPECT_TRUE(catcher.GetNextResult()) << message_;
}

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeGetPlatformInfo) {
  scoped_ptr<base::Value> result(
      extension_function_test_utils::RunFunctionAndReturnSingleResult(
          new RuntimeGetPlatformInfoFunction(),
          "[]",
          browser()));
  ASSERT_TRUE(result.get() != NULL);
  base::DictionaryValue* dict =
      extension_function_test_utils::ToDictionary(result.get());
  ASSERT_TRUE(dict != NULL);
  EXPECT_TRUE(dict->HasKey("os"));
  EXPECT_TRUE(dict->HasKey("arch"));
  EXPECT_TRUE(dict->HasKey("nacl_arch"));
}

}  // namespace extensions
