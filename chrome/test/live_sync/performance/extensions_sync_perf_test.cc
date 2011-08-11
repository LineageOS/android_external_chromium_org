// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/stringprintf.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/test/live_sync/extensions_helper.h"
#include "chrome/test/live_sync/live_sync_test.h"
#include "chrome/test/live_sync/performance/sync_timing_helper.h"

using extensions_helper::AllProfilesHaveSameExtensions;
using extensions_helper::AllProfilesHaveSameExtensionsAsVerifier;
using extensions_helper::DisableExtension;
using extensions_helper::EnableExtension;
using extensions_helper::GetInstalledExtensions;
using extensions_helper::InstallExtension;
using extensions_helper::InstallExtensionsPendingForSync;
using extensions_helper::IsExtensionEnabled;
using extensions_helper::UninstallExtension;

// TODO(braffert): Replicate these tests for apps.

// TODO(braffert): Move kNumBenchmarkPoints and kBenchmarkPoints for all
// datatypes into a performance test base class, once it is possible to do so.
static const int kNumExtensions = 150;
static const int kNumBenchmarkPoints = 18;
static const int kBenchmarkPoints[] = {1, 10, 20, 30, 40, 50, 75, 100, 125,
                                       150, 175, 200, 225, 250, 300, 350, 400,
                                       500};

class ExtensionsSyncPerfTest : public LiveSyncTest {
 public:
  ExtensionsSyncPerfTest()
      : LiveSyncTest(TWO_CLIENT),
        extension_number_(0) {}

  // Adds |num_extensions| new unique extensions to |profile|.
  void AddExtensions(int profile, int num_extensions);

  // Updates the enabled/disabled state for all extensions in |profile|.
  void UpdateExtensions(int profile);

  // Uninstalls all currently installed extensions from |profile|.
  void RemoveExtensions(int profile);

  // Returns the number of currently installed extensions for |profile|.
  int GetExtensionCount(int profile);

  // Uninstalls all extensions from all profiles.  Called between benchmark
  // iterations.
  void Cleanup();

 private:
  int extension_number_;
  DISALLOW_COPY_AND_ASSIGN(ExtensionsSyncPerfTest);
};

void ExtensionsSyncPerfTest::AddExtensions(int profile, int num_extensions) {
  for (int i = 0; i < num_extensions; ++i) {
    InstallExtension(GetProfile(profile), extension_number_++);
  }
}

void ExtensionsSyncPerfTest::UpdateExtensions(int profile) {
  std::vector<int> extensions = GetInstalledExtensions(GetProfile(profile));
  for (std::vector<int>::iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    if (IsExtensionEnabled(GetProfile(profile), *it)) {
      DisableExtension(GetProfile(profile), *it);
    } else {
      EnableExtension(GetProfile(profile), *it);
    }
  }
}

int ExtensionsSyncPerfTest::GetExtensionCount(int profile) {
  return GetInstalledExtensions(GetProfile(profile)).size();
}

void ExtensionsSyncPerfTest::RemoveExtensions(int profile) {
  std::vector<int> extensions = GetInstalledExtensions(GetProfile(profile));
  for (std::vector<int>::iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    UninstallExtension(GetProfile(profile), *it);
  }
}

void ExtensionsSyncPerfTest::Cleanup() {
  for (int i = 0; i < num_clients(); ++i) {
    RemoveExtensions(i);
  }
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AllProfilesHaveSameExtensions());
}

IN_PROC_BROWSER_TEST_F(ExtensionsSyncPerfTest, P0) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  int num_default_extensions = GetExtensionCount(0);
  int expected_extension_count = num_default_extensions + kNumExtensions;

  // TCM ID - 7563874.
  AddExtensions(0, kNumExtensions);
  base::TimeDelta dt =
      SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  InstallExtensionsPendingForSync(GetProfile(1));
  ASSERT_EQ(expected_extension_count, GetExtensionCount(1));
  SyncTimingHelper::PrintResult("extensions", "add_extensions", dt);

  // TCM ID - 7655397.
  UpdateExtensions(0);
  dt = SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(expected_extension_count, GetExtensionCount(1));
  SyncTimingHelper::PrintResult("extensions", "update_extensions", dt);

  // TCM ID - 7567721.
  RemoveExtensions(0);
  dt = SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(num_default_extensions, GetExtensionCount(1));
  SyncTimingHelper::PrintResult("extensions", "delete_extensions", dt);
}

IN_PROC_BROWSER_TEST_F(ExtensionsSyncPerfTest, DISABLED_Benchmark) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  for (int i = 0; i < kNumBenchmarkPoints; ++i) {
    int num_extensions = kBenchmarkPoints[i];
    AddExtensions(0, num_extensions);
    base::TimeDelta dt_add =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    InstallExtensionsPendingForSync(GetProfile(1));
    VLOG(0) << std::endl << "Add: " << num_extensions << " "
            << dt_add.InSecondsF();

    UpdateExtensions(0);
    base::TimeDelta dt_update =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    VLOG(0) << std::endl << "Update: " << num_extensions << " "
            << dt_update.InSecondsF();

    RemoveExtensions(0);
    base::TimeDelta dt_delete =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    VLOG(0) << std::endl << "Delete: " << num_extensions << " "
            << dt_delete.InSecondsF();

    Cleanup();
  }
}
