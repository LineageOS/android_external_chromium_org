// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_
#define CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_

#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "chrome/browser/prefs/synced_pref_change_registrar.h"
#include "chrome/browser/profiles/profile.h"
#include "components/browser_context_keyed_service/browser_context_keyed_service.h"
#include "components/browser_context_keyed_service/browser_context_keyed_service_factory.h"

// PrefMetricsService is responsible for recording prefs-related UMA stats.
class PrefMetricsService : public BrowserContextKeyedService {
 public:
  explicit PrefMetricsService(Profile* profile);
  virtual ~PrefMetricsService();

  class Factory : public BrowserContextKeyedServiceFactory {
   public:
    static Factory* GetInstance();
    static PrefMetricsService* GetForProfile(Profile* profile);
   private:
    friend struct DefaultSingletonTraits<Factory>;

    Factory();
    virtual ~Factory();

    // BrowserContextKeyedServiceFactory implementation
    virtual BrowserContextKeyedService* BuildServiceInstanceFor(
        content::BrowserContext* profile) const OVERRIDE;
    virtual bool ServiceIsCreatedWithBrowserContext() const OVERRIDE;
    virtual bool ServiceIsNULLWhileTesting() const OVERRIDE;
    virtual content::BrowserContext* GetBrowserContextToUse(
        content::BrowserContext* context) const OVERRIDE;
  };

 private:
  // Use a map to convert domains to their histogram identifiers. Ids are
  // defined in tools/metrics/histograms/histograms.xml and (usually) also in
  // chrome/browser/search_engines/prepopulated_engines.json.
  typedef std::map<std::string, int> DomainIdMap;

  // Function to log a Value to a histogram
  typedef base::Callback<void(const std::string&, const Value*)>
      LogHistogramValueCallback;

  // Record prefs state on browser context creation.
  void RecordLaunchPrefs();

  // Register callbacks for synced pref changes.
  void RegisterSyncedPrefObservers();

  // Registers a histogram logging callback for a synced pref change.
  void AddPrefObserver(const std::string& path,
                       const std::string& histogram_name_prefix,
                       const LogHistogramValueCallback& callback);

  // Generic callback to observe a synced pref change.
  void OnPrefChanged(const std::string& histogram_name_prefix,
                     const LogHistogramValueCallback& callback,
                     const std::string& path,
                     bool from_sync);

  // Callback for a boolean pref change histogram.
  void LogBooleanPrefChange(const std::string& histogram_name,
                            const Value* value);

  // Callback for an integer pref change histogram.
  void LogIntegerPrefChange(int boundary_value,
                            const std::string& histogram_name,
                            const Value* value);

  // Callback for a list pref change. Each item in the list
  // is logged using the given histogram callback.
  void LogListPrefChange(const LogHistogramValueCallback& item_callback,
                         const std::string& histogram_name,
                         const Value* value);

  Profile* profile_;
  scoped_ptr<SyncedPrefChangeRegistrar> synced_pref_change_registrar_;
};

#endif  // CHROME_BROWSER_PREFS_PREF_METRICS_SERVICE_H_
