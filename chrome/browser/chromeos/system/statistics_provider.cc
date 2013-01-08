// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system/statistics_provider.h"

#include "base/bind.h"
#include "base/chromeos/chromeos_version.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "base/time.h"
#include "chrome/browser/chromeos/system/name_value_pairs_parser.h"
#include "chrome/common/child_process_logging.h"
#include "chrome/common/chrome_version_info.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {
namespace system {
namespace {

// Path to the tool used to get system info, and delimiters for the output
// format of the tool.
const char* kCrosSystemTool[] = { "/usr/bin/crossystem" };
const char kCrosSystemEq[] = "=";
const char kCrosSystemDelim[] = "\n";
const char kCrosSystemCommentDelim[] = "#";
const char kCrosSystemUnknownValue[] = "(error)";

const char kHardwareClassCrosSystemKey[] = "hwid";
const char kHardwareClassKey[] = "hardware_class";
const char kUnknownHardwareClass[] = "unknown";

// File to get machine hardware info from, and key/value delimiters of
// the file.
// /tmp/machine-info is generated by platform/init/chromeos_startup.
const char kMachineHardwareInfoFile[] = "/tmp/machine-info";
const char kMachineHardwareInfoEq[] = "=";
const char kMachineHardwareInfoDelim[] = " \n";

// File to get ECHO coupon info from, and key/value delimiters of
// the file.
const char kEchoCouponFile[] = "/var/cache/echo/vpd_echo.txt";
const char kEchoCouponEq[] = "=";
const char kEchoCouponDelim[] = "\n";

// File to get machine OS info from, and key/value delimiters of the file.
const char kMachineOSInfoFile[] = "/etc/lsb-release";
const char kMachineOSInfoEq[] = "=";
const char kMachineOSInfoDelim[] = "\n";

// File to get VPD info from, and key/value delimiters of the file.
const char kVpdFile[] = "/var/log/vpd_2.0.txt";
const char kVpdEq[] = "=";
const char kVpdDelim[] = "\n";

// Timeout that we should wait for statistics to get loaded
const int kTimeoutSecs = 3;

}  // namespace

// The StatisticsProvider implementation used in production.
class StatisticsProviderImpl : public StatisticsProvider {
 public:
  // StatisticsProvider implementation:
  virtual void Init() OVERRIDE;
  virtual void StartLoadingMachineStatistics() OVERRIDE;
  virtual bool GetMachineStatistic(const std::string& name,
                                   std::string* result) OVERRIDE;

  static StatisticsProviderImpl* GetInstance();

 private:
  friend struct DefaultSingletonTraits<StatisticsProviderImpl>;

  StatisticsProviderImpl();

  // Loads the machine info file, which is necessary to get the Chrome channel.
  // Treat MachineOSInfoFile specially, as distribution channel information
  // (stable, beta, dev, canary) is required at earlier stage than everything
  // else. Rather than posting a delayed task, read and parse the machine OS
  // info file immediately.
  void LoadMachineOSInfoFile();

  // Loads the machine statistcs by examining the system.
  void LoadMachineStatistics();

  bool initialized_;
  bool load_statistics_started_;
  NameValuePairsParser::NameValueMap machine_info_;
  base::WaitableEvent on_statistics_loaded_;

  DISALLOW_COPY_AND_ASSIGN(StatisticsProviderImpl);
};

void StatisticsProviderImpl::Init() {
  DCHECK(!initialized_);
  initialized_ = true;

  // Load the machine info file immediately to get the channel info.
  LoadMachineOSInfoFile();
}

bool StatisticsProviderImpl::GetMachineStatistic(
    const std::string& name, std::string* result) {
  DCHECK(initialized_);
  DCHECK(load_statistics_started_);

  VLOG(1) << "Statistic is requested for " << name;
  // Block if the statistics are not loaded yet. Per LOG(WARNING) below,
  // the statistics are loaded before requested as of now. For regular
  // sessions (i.e. not OOBE), statistics are first requested when the
  // user is logging in so we have plenty of time to load the data
  // beforehand.
  //
  // If you see the warning appeared for regular sessions, it probably
  // means that there is new client code that uses the statistics in the
  // very early stage of the browser startup. The statistic name should be
  // helpful to identify the caller.
  if (!on_statistics_loaded_.IsSignaled()) {
    LOG(WARNING) << "Waiting to load statistics. Requested statistic: "
                 << name;
    // http://crbug.com/125385
    base::ThreadRestrictions::ScopedAllowWait allow_wait;
    on_statistics_loaded_.TimedWait(base::TimeDelta::FromSeconds(kTimeoutSecs));

    if (!on_statistics_loaded_.IsSignaled()) {
      LOG(ERROR) << "Statistics weren't loaded after waiting! "
                 << "Requested statistic: " << name;
      return false;
    }
  }

  NameValuePairsParser::NameValueMap::iterator iter = machine_info_.find(name);
  if (iter != machine_info_.end()) {
    *result = iter->second;
    return true;
  }
  return false;
}

// manual_reset needs to be true, as we want to keep the signaled state.
StatisticsProviderImpl::StatisticsProviderImpl()
    : initialized_(false),
      load_statistics_started_(false),
      on_statistics_loaded_(true  /* manual_reset */,
                            false /* initially_signaled */) {
}

void StatisticsProviderImpl::LoadMachineOSInfoFile() {
  NameValuePairsParser parser(&machine_info_);
  if (parser.GetNameValuePairsFromFile(FilePath(kMachineOSInfoFile),
                                       kMachineOSInfoEq,
                                       kMachineOSInfoDelim)) {
#if defined(GOOGLE_CHROME_BUILD)
    const char kChromeOSReleaseTrack[] = "CHROMEOS_RELEASE_TRACK";
    NameValuePairsParser::NameValueMap::iterator iter =
        machine_info_.find(kChromeOSReleaseTrack);
    if (iter != machine_info_.end())
      chrome::VersionInfo::SetChannel(iter->second);
#endif
  }
}

void StatisticsProviderImpl::StartLoadingMachineStatistics() {
  DCHECK(initialized_);
  DCHECK(!load_statistics_started_);
  load_statistics_started_ = true;

  VLOG(1) << "Started loading statistics";
  BrowserThread::PostBlockingPoolTask(
      FROM_HERE,
      base::Bind(&StatisticsProviderImpl::LoadMachineStatistics,
                 base::Unretained(this)));
}

void StatisticsProviderImpl::LoadMachineStatistics() {
  NameValuePairsParser parser(&machine_info_);

  // Parse all of the key/value pairs from the crossystem tool.
  if (!parser.ParseNameValuePairsFromTool(
          arraysize(kCrosSystemTool), kCrosSystemTool, kCrosSystemEq,
          kCrosSystemDelim, kCrosSystemCommentDelim)) {
    LOG(WARNING) << "There were errors parsing the output of "
                 << kCrosSystemTool << ".";
  }

  // Ensure that the hardware class key is present with the expected
  // key name, and if it couldn't be retrieved, that the value is "unknown".
  std::string hardware_class = machine_info_[kHardwareClassCrosSystemKey];
  if (hardware_class.empty() || hardware_class == kCrosSystemUnknownValue)
    machine_info_[kHardwareClassKey] = kUnknownHardwareClass;
  else
    machine_info_[kHardwareClassKey] = hardware_class;

  parser.GetNameValuePairsFromFile(FilePath(kMachineHardwareInfoFile),
                                   kMachineHardwareInfoEq,
                                   kMachineHardwareInfoDelim);
  parser.GetNameValuePairsFromFile(FilePath(kEchoCouponFile),
                                   kEchoCouponEq,
                                   kEchoCouponDelim);
  parser.GetNameValuePairsFromFile(FilePath(kVpdFile), kVpdEq, kVpdDelim);

  // Finished loading the statistics.
  on_statistics_loaded_.Signal();
  VLOG(1) << "Finished loading statistics";
}

StatisticsProviderImpl* StatisticsProviderImpl::GetInstance() {
  return Singleton<StatisticsProviderImpl,
                   DefaultSingletonTraits<StatisticsProviderImpl> >::get();
}

// The stub StatisticsProvider implementation used on Linux desktop.
class StatisticsProviderStubImpl : public StatisticsProvider {
 public:
  // StatisticsProvider implementation:
  virtual void Init() OVERRIDE {}

  virtual void StartLoadingMachineStatistics() OVERRIDE {}

  virtual bool GetMachineStatistic(const std::string& name,
                                   std::string* result) OVERRIDE {
    if (name == "CHROMEOS_RELEASE_BOARD") {
      // Note: syncer::GetSessionNameSynchronously() also uses the mechanism
      // below to determine the CrOs release board. However, it cannot include
      // statistics_provider.h and use this method because of the mutual
      // dependency that creates between sync.gyp:sync and chrome.gyp:browser.
      // TODO(rsimha): Update syncer::GetSessionNameSynchronously() if this code
      // is ever moved into base/. See http://crbug.com/126732.
      const CommandLine* command_line = CommandLine::ForCurrentProcess();
      if (command_line->HasSwitch(chromeos::switches::kChromeOSReleaseBoard)) {
        *result = command_line->
            GetSwitchValueASCII(chromeos::switches::kChromeOSReleaseBoard);
        return true;
      }
    }
    return false;
  }

  static StatisticsProviderStubImpl* GetInstance() {
    return Singleton<StatisticsProviderStubImpl,
        DefaultSingletonTraits<StatisticsProviderStubImpl> >::get();
  }

 private:
  friend struct DefaultSingletonTraits<StatisticsProviderStubImpl>;

  StatisticsProviderStubImpl() {
  }

  DISALLOW_COPY_AND_ASSIGN(StatisticsProviderStubImpl);
};

StatisticsProvider* StatisticsProvider::GetInstance() {
  if (base::chromeos::IsRunningOnChromeOS()) {
    return StatisticsProviderImpl::GetInstance();
  } else {
    return StatisticsProviderStubImpl::GetInstance();
  }
}

}  // namespace system
}  // namespace chromeos
