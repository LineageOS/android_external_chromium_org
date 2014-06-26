// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_PROVIDER_LOGOS_LOGO_TRACKER_H_
#define COMPONENTS_SEARCH_PROVIDER_LOGOS_LOGO_TRACKER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequenced_task_runner.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "components/search_provider_logos/logo_cache.h"
#include "components/search_provider_logos/logo_common.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace search_provider_logos {

// Receives updates when the search provider's logo is available.
class LogoObserver {
 public:
  virtual ~LogoObserver() {}

  // Called when the cached logo is available and possibly when a freshly
  // downloaded logo is available. |logo| will be NULL if no logo is available.
  // |from_cache| indicates whether the logo was loaded from the cache.
  //
  // If the fresh logo is the same as the cached logo, this will not be called
  // again.
  virtual void OnLogoAvailable(const Logo* logo, bool from_cache) = 0;

  // Called when the LogoTracker will no longer send updates to this
  // LogoObserver. For example: after the cached logo is validated, after
  // OnFreshLogoAvailable() is called, or when the LogoTracker is destructed.
  // This is not called when an observer is removed using
  // LogoTracker::RemoveObserver().
  virtual void OnObserverRemoved() = 0;
};

// Provides a LogoTracker with methods it needs to download and cache logos.
class LogoDelegate {
 public:
  virtual ~LogoDelegate() {}

  // Decodes an untrusted image safely and returns it as an SkBitmap via
  // |image_decoded_callback|. If image decoding fails, |image_decoded_callback|
  // should be called with NULL. This will be called on the thread that
  // LogoTracker lives on and |image_decoded_callback| must be called on the
  // same thread.
  virtual void DecodeUntrustedImage(
      const scoped_refptr<base::RefCountedString>& encoded_image,
      base::Callback<void(const SkBitmap&)> image_decoded_callback) = 0;
};

// Parses the response from the server and returns it as an EncodedLogo. Returns
// NULL if the response is invalid.
typedef base::Callback<
    scoped_ptr<EncodedLogo>(const scoped_ptr<std::string>& response,
                            base::Time response_time)> ParseLogoResponse;

// Encodes the fingerprint of the cached logo in the logo URL. This enables the
// server to verify whether the cached logo is up-to-date.
typedef base::Callback<
    GURL(const GURL& logo_url, const std::string& fingerprint)>
    AppendFingerprintToLogoURL;

// This class provides the logo for a search provider. Logos are downloaded from
// the search provider's logo URL and cached on disk.
//
// Call SetServerAPI() at least once to specify how to get the logo from the
// server. Then call GetLogo() to trigger retrieval of the logo and receive
// updates once the cached and/or fresh logos are available.
class LogoTracker : public net::URLFetcherDelegate {
 public:
  // Constructs a LogoTracker with the given LogoDelegate. Takes ownership of
  // |delegate|, which will be deleted at the same time as the LogoTracker.
  //
  // |cached_logo_directory| is the directory in which the cached logo and its
  // metadata should be saved.
  //
  // |file_task_runner| is the SequencedTaskRunner that should be used to run
  // file system operations.
  //
  // |background_task_runner| is the TaskRunner that should be used to for
  // CPU-intensive background operations.
  //
  // |request_context_getter| is the URLRequestContextGetter used to download
  // the logo.
  explicit LogoTracker(
      base::FilePath cached_logo_directory,
      scoped_refptr<base::SequencedTaskRunner> file_task_runner,
      scoped_refptr<base::TaskRunner> background_task_runner,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      scoped_ptr<LogoDelegate> delegate);

  virtual ~LogoTracker();

  // Defines the server API for downloading and parsing the logo. This must be
  // called at least once before calling GetLogo().
  //
  // |logo_url| is the URL from which the logo will be downloaded. If |logo_url|
  // is different than the current logo URL, any pending LogoObservers will be
  // canceled.
  //
  // |parse_logo_response_func| is a callback that will be used to parse the
  // server's response into a EncodedLogo object. |append_fingerprint_func| is a
  // callback that will return the URL from which to download the logo if a
  // cached logo with a fingerprint is already available. Note:
  // |parse_logo_response_func| and |append_fingerprint_func| must be suitable
  // for running multiple times, concurrently, and on multiple threads.
  void SetServerAPI(const GURL& logo_url,
                    const ParseLogoResponse& parse_logo_response_func,
                    const AppendFingerprintToLogoURL& append_fingerprint_func);

  // Retrieves the current search provider's logo from the local cache and/or
  // over the network, and registers |observer| to be called when the cached
  // and/or fresh logos are available.
  void GetLogo(LogoObserver* observer);

  // Prevents |observer| from receiving future updates. This is safe to call
  // even when the observer is being notified of an update.
  void RemoveObserver(LogoObserver* observer);

  // Overrides the cache used to store logos.
  void SetLogoCacheForTests(scoped_ptr<LogoCache> cache);

  // Overrides the clock used to check the time.
  void SetClockForTests(scoped_ptr<base::Clock> clock);

 private:
  // Cancels the current asynchronous operation, if any, and resets all member
  // variables that change as the logo is fetched.
  void ReturnToIdle();

  // Called when the cached logo has been read from the cache. |cached_logo|
  // will be NULL if there wasn't a valid, up-to-date logo in the cache.
  void OnCachedLogoRead(scoped_ptr<EncodedLogo> cached_logo);

  // Called when the cached logo has been decoded into an SkBitmap. |image| will
  // be NULL if decoding failed.
  void OnCachedLogoAvailable(const LogoMetadata& metadata,
                             const SkBitmap& image);

  // Stores |logo| in the cache.
  void SetCachedLogo(scoped_ptr<EncodedLogo> logo);

  // Updates the metadata for the logo already stored in the cache.
  void SetCachedMetadata(const LogoMetadata& metadata);

  // Starts fetching the current logo over the network.
  void FetchLogo();

  // Called when the logo has been downloaded and parsed. |logo| will be NULL
  // if the server's response was invalid.
  void OnFreshLogoParsed(scoped_ptr<EncodedLogo> logo);

  // Called when the fresh logo has been decoded into an SkBitmap. |image| will
  // be NULL if decoding failed.
  void OnFreshLogoAvailable(scoped_ptr<EncodedLogo> logo,
                            const SkBitmap& image);

  // net::URLFetcherDelegate:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;
  virtual void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                          int64 current,
                                          int64 total) OVERRIDE;

  // The URL from which the logo is fetched.
  GURL logo_url_;

  // The function used to parse the logo response from the server.
  ParseLogoResponse parse_logo_response_func_;

  // The function used to include the cached logo's fingerprint in the logo URL.
  AppendFingerprintToLogoURL append_fingerprint_func_;

  // False if an asynchronous task is currently running.
  bool is_idle_;

  // The logo that's been read from the cache, or NULL if the cache is empty.
  // Meaningful only if is_cached_logo_valid_ is true; NULL otherwise.
  scoped_ptr<Logo> cached_logo_;

  // Whether the value of |cached_logo_| reflects the actual cached logo.
  // This will be false if the logo hasn't been read from the cache yet.
  // |cached_logo_| may be NULL even if |is_cached_logo_valid_| is true, if no
  // logo is cached.
  bool is_cached_logo_valid_;

  // The URLFetcher currently fetching the logo. NULL when not fetching.
  scoped_ptr<net::URLFetcher> fetcher_;

  // The list of observers to be notified when the logo is available. This
  // should be empty when the state is IDLE.
  ObserverList<LogoObserver> logo_observers_;

  scoped_ptr<LogoDelegate> logo_delegate_;

  // The cache used to persist the logo on disk. Used only on the file thread.
  LogoCache* logo_cache_;

  // Clock used to determine current time. Can be overridden in tests.
  scoped_ptr<base::Clock> clock_;

  // The SequencedTaskRunner on which file system operations will be run.
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  // The TaskRunner on which the server's response will be parsed.
  scoped_refptr<base::TaskRunner> background_task_runner_;

  // The URLRequestContextGetter used for network requests.
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  base::WeakPtrFactory<LogoTracker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LogoTracker);
};

}  // namespace search_provider_logos

#endif  // COMPONENTS_SEARCH_PROVIDER_LOGOS_LOGO_TRACKER_H_
