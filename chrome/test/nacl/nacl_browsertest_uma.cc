// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/uma_histogram_helper.h"
#include "chrome/test/nacl/nacl_browsertest_util.h"
#include "components/nacl/browser/nacl_browser.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "ppapi/c/private/ppb_nacl_private.h"

namespace {

NACL_BROWSER_TEST_F(NaClBrowserTest, SuccessfulLoadUMA, {
  // Load a NaCl module to generate UMA data.
  RunLoadTest(FILE_PATH_LITERAL("nacl_load_test.html"));

  // Make sure histograms from child processes have been accumulated in the
  // browser brocess.
  UMAHistogramHelper histograms;
  histograms.Fetch();

  // Did the plugin report success?
  histograms.ExpectUniqueSample("NaCl.LoadStatus.Plugin",
                                PP_NACL_ERROR_LOAD_SUCCESS, 1);

  // Did the sel_ldr report success?
  histograms.ExpectUniqueSample("NaCl.LoadStatus.SelLdr",
                                LOAD_OK, 1);

  // Check validation cache usage:
  // For the open-web, only the IRT is considered a "safe" and
  // identity-cachable file. The nexes and .so files are not.
  // Should have one cache query for the IRT.
  histograms.ExpectBucketCount("NaCl.ValidationCache.Query",
                               nacl::NaClBrowser::CACHE_MISS, 1);
  // TOTAL should then be 1 query so far.
  histograms.ExpectTotalCount("NaCl.ValidationCache.Query", 1);
  // Should have received a cache setting afterwards for IRT.
  histograms.ExpectTotalCount("NaCl.ValidationCache.Set", 1);

  // Make sure we have other important histograms.
  if (!IsAPnaclTest()) {
    histograms.ExpectTotalCount("NaCl.Perf.StartupTime.LoadModule", 1);
    histograms.ExpectTotalCount("NaCl.Perf.StartupTime.Total", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.Manifest", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.Nexe", 1);
  } else {
    histograms.ExpectTotalCount("NaCl.Options.PNaCl.OptLevel", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.Manifest", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.Pexe", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.PNaClTranslatedNexe", 1);
    histograms.ExpectTotalCount("NaCl.Perf.Size.PexeNexeSizePct", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.LoadCompiler", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.LoadLinker", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.CompileTime", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.CompileKBPerSec", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.LinkTime", 1);
    histograms.ExpectTotalCount(
        "NaCl.Perf.PNaClLoadTime.PctCompiledWhenFullyDownloaded", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClLoadTime.TotalUncachedTime", 1);
    histograms.ExpectTotalCount(
        "NaCl.Perf.PNaClLoadTime.TotalUncachedKBPerSec", 1);
    histograms.ExpectTotalCount("NaCl.Perf.PNaClCache.IsHit", 1);
  }
})

class NaClBrowserTestVcacheExtension:
      public NaClBrowserTestNewlibExtension {
 public:
  virtual base::FilePath::StringType Variant() OVERRIDE {
    return FILE_PATH_LITERAL("extension_vcache_test/newlib");
  }
};

IN_PROC_BROWSER_TEST_F(NaClBrowserTestVcacheExtension,
                       ValidationCacheOfMainNexe) {
  // Hardcoded extension AppID that corresponds to the hardcoded
  // public key in the manifest.json file. We need to load the extension
  // nexe from the same origin, so we can't just try to load the extension
  // nexe as a mime-type handler from a non-extension URL.
  base::FilePath::StringType full_url =
      FILE_PATH_LITERAL("chrome-extension://cbcdidchbppangcjoddlpdjlenngjldk/")
      FILE_PATH_LITERAL("extension_validation_cache.html");
  RunNaClIntegrationTest(full_url, true);

  // Make sure histograms from child processes have been accumulated in the
  // browser brocess.
  UMAHistogramHelper histograms;
  histograms.Fetch();
  // Should have received 2 validation queries (one for IRT and one for NEXE),
  // and responded with a miss.
  histograms.ExpectBucketCount("NaCl.ValidationCache.Query",
                               nacl::NaClBrowser::CACHE_MISS, 2);
  // TOTAL should then be 2 queries so far.
  histograms.ExpectTotalCount("NaCl.ValidationCache.Query", 2);
  // Should have received a cache setting afterwards for IRT and nexe.
  histograms.ExpectBucketCount("NaCl.ValidationCache.Set",
                               nacl::NaClBrowser::CACHE_HIT, 2);

  // Load it again to hit the cache.
  RunNaClIntegrationTest(full_url, true);
  histograms.Fetch();
  // Should have received 2 more validation queries later (IRT and NEXE),
  // and responded with a hit.
  histograms.ExpectBucketCount("NaCl.ValidationCache.Query",
                               nacl::NaClBrowser::CACHE_HIT, 2);
  // TOTAL should then be 4 queries now.
  histograms.ExpectTotalCount("NaCl.ValidationCache.Query", 4);
  // Still only 2 settings.
  histograms.ExpectTotalCount("NaCl.ValidationCache.Set", 2);
}

// TODO(ncbray) convert the rest of nacl_uma.py (currently in the NaCl repo.)
// Test validation failures and crashes.

}  // namespace
