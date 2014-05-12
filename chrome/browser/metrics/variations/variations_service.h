// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_VARIATIONS_VARIATIONS_SERVICE_H_
#define CHROME_BROWSER_METRICS_VARIATIONS_VARIATIONS_SERVICE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/time/time.h"
#include "chrome/browser/metrics/variations/variations_request_scheduler.h"
#include "chrome/browser/metrics/variations/variations_seed_store.h"
#include "chrome/browser/web_resource/resource_request_allowed_notifier.h"
#include "chrome/common/chrome_version_info.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "chrome/browser/metrics/variations/variations_registry_syncer_win.h"
#endif

class PrefService;
class PrefRegistrySimple;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace metrics {
class MetricsStateManager;
}

namespace chrome_variations {

class VariationsSeed;

// Used to setup field trials based on stored variations seed data, and fetch
// new seed data from the variations server.
class VariationsService
    : public net::URLFetcherDelegate,
      public ResourceRequestAllowedNotifier::Observer {
 public:
  virtual ~VariationsService();

  // Creates field trials based on Variations Seed loaded from local prefs. If
  // there is a problem loading the seed data, all trials specified by the seed
  // may not be created.
  bool CreateTrialsFromSeed();

  // Calls FetchVariationsSeed once and repeats this periodically. See
  // implementation for details on the period. Must be called after
  // |CreateTrialsFromSeed|.
  void StartRepeatedVariationsSeedFetch();

  // Returns the variations server URL, which can vary if a command-line flag is
  // set and/or the variations restrict pref is set in |local_prefs|. Declared
  // static for test purposes.
  static GURL GetVariationsServerURL(PrefService* local_prefs);

  // Called when the application enters foreground. This may trigger a
  // FetchVariationsSeed call.
  // TODO(rkaplow): Handle this and the similar event in metrics_service by
  // observing an 'OnAppEnterForeground' event instead of requiring the frontend
  // code to notify each service individually.
  void OnAppEnterForeground();

#if defined(OS_WIN)
  // Starts syncing Google Update Variation IDs with the registry.
  void StartGoogleUpdateRegistrySync();
#endif

  // Exposed for testing.
  void SetCreateTrialsFromSeedCalledForTesting(bool called);

  // Exposed for testing.
  static std::string GetDefaultVariationsServerURLForTesting();

  // Register Variations related prefs in Local State.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Register Variations related prefs in the Profile prefs.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Factory method for creating a VariationsService. Does not take ownership of
  // |state_manager|. Caller should ensure that |state_manager| is valid for the
  // lifetime of this class.
  static scoped_ptr<VariationsService> Create(
      PrefService* local_state,
      metrics::MetricsStateManager* state_manager);

  // Set the PrefService responsible for getting policy-related preferences,
  // such as the restrict parameter.
  void set_policy_pref_service(PrefService* service) {
    DCHECK(service);
    policy_pref_service_ = service;
  }

 protected:
  // Starts the fetching process once, where |OnURLFetchComplete| is called with
  // the response.
  virtual void DoActualFetch();

  // Stores the seed to prefs. Set as virtual and protected so that it can be
  // overridden by tests.
  virtual void StoreSeed(const std::string& seed_data,
                         const std::string& seed_signature,
                         const base::Time& date_fetched);

  // This constructor exists for injecting a mock notifier. It is meant for
  // testing only. This instance will take ownership of |notifier|. Does not
  // take ownership of |state_manager|. Caller should ensure that
  // |state_manager| is valid for the lifetime of this class.
  VariationsService(ResourceRequestAllowedNotifier* notifier,
                    PrefService* local_state,
                    metrics::MetricsStateManager* state_manager);

 private:
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, DoNotFetchIfOffline);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, DoNotFetchIfOnlineToOnline);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, FetchOnReconnect);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, LoadSeed);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, StoreSeed);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, SeedStoredWhenOKStatus);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, SeedNotStoredWhenNonOKStatus);
  FRIEND_TEST_ALL_PREFIXES(VariationsServiceTest, SeedDateUpdatedOn304Status);

  // Creates the VariationsService with the given |local_state| prefs service
  // and |state_manager|. Does not take ownership of |state_manager|. Caller
  // should ensure that |state_manager| is valid for the lifetime of this class.
  // Use the |Create| factory method to create a VariationsService.
  VariationsService(PrefService* local_state,
                    metrics::MetricsStateManager* state_manager);

  // Checks if prerequisites for fetching the Variations seed are met, and if
  // so, performs the actual fetch using |DoActualFetch|.
  void FetchVariationsSeed();

  // net::URLFetcherDelegate implementation:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

  // ResourceRequestAllowedNotifier::Observer implementation:
  virtual void OnResourceRequestsAllowed() OVERRIDE;

  // Record the time of the most recent successful fetch.
  void RecordLastFetchTime();

  // The pref service used to store persist the variations seed.
  PrefService* local_state_;

  // Used for instantiating entropy providers for variations seed simulation.
  // Weak pointer.
  metrics::MetricsStateManager* state_manager_;

  // Used to obtain policy-related preferences. Depending on the platform, will
  // either be Local State or Profile prefs.
  PrefService* policy_pref_service_;

  VariationsSeedStore seed_store_;

  // Contains the scheduler instance that handles timing for requests to the
  // server. Initially NULL and instantiated when the initial fetch is
  // requested.
  scoped_ptr<VariationsRequestScheduler> request_scheduler_;

  // Contains the current seed request. Will only have a value while a request
  // is pending, and will be reset by |OnURLFetchComplete|.
  scoped_ptr<net::URLFetcher> pending_seed_request_;

  // The URL to use for querying the Variations server.
  GURL variations_server_url_;

  // Tracks whether |CreateTrialsFromSeed| has been called, to ensure that
  // it gets called prior to |StartRepeatedVariationsSeedFetch|.
  bool create_trials_from_seed_called_;

  // Tracks whether the initial request to the variations server had completed.
  bool initial_request_completed_;

  // Helper class used to tell this service if it's allowed to make network
  // resource requests.
  scoped_ptr<ResourceRequestAllowedNotifier> resource_request_allowed_notifier_;

  // The start time of the last seed request. This is used to measure the
  // latency of seed requests. Initially zero.
  base::TimeTicks last_request_started_time_;

#if defined(OS_WIN)
  // Helper that handles synchronizing Variations with the Registry.
  VariationsRegistrySyncer registry_syncer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(VariationsService);
};

}  // namespace chrome_variations

#endif  // CHROME_BROWSER_METRICS_VARIATIONS_VARIATIONS_SERVICE_H_
