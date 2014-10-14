// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"

#include "base/memory/weak_ptr.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/load_flags.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace {

const size_t kMaxRequests = 25;  // Maximum number of inflight requests allowed.
const int kMaxCacheEntries = 5;  // Maximum number of cache entries.

}  // namespace.

class BitmapFetcherRequest {
 public:
  BitmapFetcherRequest(BitmapFetcherService::RequestId request_id,
                       BitmapFetcherService::Observer* observer);
  ~BitmapFetcherRequest();

  void NotifyImageChanged(const SkBitmap& bitmap);
  BitmapFetcherService::RequestId request_id() const { return request_id_; }

  // Weak ptr |fetcher| is used to identify associated fetchers.
  void set_fetcher(const chrome::BitmapFetcher* fetcher) { fetcher_ = fetcher; }
  const chrome::BitmapFetcher* get_fetcher() const { return fetcher_; }

 private:
  const BitmapFetcherService::RequestId request_id_;
  scoped_ptr<BitmapFetcherService::Observer> observer_;
  const chrome::BitmapFetcher* fetcher_;

  DISALLOW_COPY_AND_ASSIGN(BitmapFetcherRequest);
};

BitmapFetcherRequest::BitmapFetcherRequest(
    BitmapFetcherService::RequestId request_id,
    BitmapFetcherService::Observer* observer)
    : request_id_(request_id), observer_(observer) {
}

BitmapFetcherRequest::~BitmapFetcherRequest() {
}

void BitmapFetcherRequest::NotifyImageChanged(const SkBitmap& bitmap) {
  if (!bitmap.empty())
    observer_->OnImageChanged(request_id_, bitmap);
}

BitmapFetcherService::CacheEntry::CacheEntry() {
}

BitmapFetcherService::CacheEntry::~CacheEntry() {
}

BitmapFetcherService::BitmapFetcherService(content::BrowserContext* context)
    : cache_(kMaxCacheEntries), current_request_id_(1), context_(context) {
}

BitmapFetcherService::~BitmapFetcherService() {
}

void BitmapFetcherService::CancelRequest(int request_id) {
  ScopedVector<BitmapFetcherRequest>::iterator iter;
  for (iter = requests_.begin(); iter != requests_.end(); ++iter) {
    if ((*iter)->request_id() == request_id) {
      requests_.erase(iter);
      // Deliberately leave the associated fetcher running to populate cache.
      return;
    }
  }
}

BitmapFetcherService::RequestId BitmapFetcherService::RequestImage(
    const GURL& url,
    Observer* observer) {
  // Create a new request, assigning next available request ID.
  ++current_request_id_;
  if (current_request_id_ == REQUEST_ID_INVALID)
    ++current_request_id_;
  int request_id = current_request_id_;
  scoped_ptr<BitmapFetcherRequest> request(
      new BitmapFetcherRequest(request_id, observer));

  // Reject invalid URLs.
  if (!url.is_valid())
    return REQUEST_ID_INVALID;

  // Check for existing images first.
  base::OwningMRUCache<GURL, CacheEntry*>::iterator iter = cache_.Get(url);
  if (iter != cache_.end()) {
    BitmapFetcherService::CacheEntry* entry = iter->second;
    request->NotifyImageChanged(*(entry->bitmap.get()));

    // There is no request ID associated with this - data is already delivered.
    return REQUEST_ID_INVALID;
  }

  // Limit number of simultaneous in-flight requests.
  if (requests_.size() > kMaxRequests)
    return REQUEST_ID_INVALID;

  // Make sure there's a fetcher for this URL and attach to request.
  const chrome::BitmapFetcher* fetcher = EnsureFetcherForUrl(url);
  request->set_fetcher(fetcher);

  requests_.push_back(request.release());
  return requests_.back()->request_id();
}

void BitmapFetcherService::Prefetch(const GURL& url) {
  if (url.is_valid())
    EnsureFetcherForUrl(url);
}

chrome::BitmapFetcher* BitmapFetcherService::CreateFetcher(const GURL& url) {
  chrome::BitmapFetcher* new_fetcher = new chrome::BitmapFetcher(url, this);

  new_fetcher->Start(
      context_->GetRequestContext(),
      std::string(),
      net::URLRequest::CLEAR_REFERRER_ON_TRANSITION_FROM_SECURE_TO_INSECURE,
      net::LOAD_NORMAL);
  return new_fetcher;
}

const chrome::BitmapFetcher* BitmapFetcherService::EnsureFetcherForUrl(
    const GURL& url) {
  const chrome::BitmapFetcher* fetcher = FindFetcherForUrl(url);
  if (fetcher)
    return fetcher;

  chrome::BitmapFetcher* new_fetcher = CreateFetcher(url);
  active_fetchers_.push_back(new_fetcher);
  return new_fetcher;
}

const chrome::BitmapFetcher* BitmapFetcherService::FindFetcherForUrl(
    const GURL& url) {
  for (BitmapFetchers::iterator iter = active_fetchers_.begin();
       iter != active_fetchers_.end();
       ++iter) {
    if (url == (*iter)->url())
      return *iter;
  }
  return NULL;
}

void BitmapFetcherService::RemoveFetcher(const chrome::BitmapFetcher* fetcher) {
  for (BitmapFetchers::iterator iter = active_fetchers_.begin();
       iter != active_fetchers_.end();
       ++iter) {
    if (fetcher == (*iter)) {
      active_fetchers_.erase(iter);
      return;
    }
  }
  NOTREACHED();  // RemoveFetcher should always result in removal.
}

void BitmapFetcherService::OnFetchComplete(const GURL url,
                                           const SkBitmap* bitmap) {
  DCHECK(bitmap);  // can never be NULL, guaranteed by BitmapFetcher.

  const chrome::BitmapFetcher* fetcher = FindFetcherForUrl(url);
  DCHECK(fetcher);

  // Notify all attached requests of completion.
  ScopedVector<BitmapFetcherRequest>::iterator iter = requests_.begin();
  while (iter != requests_.end()) {
    if ((*iter)->get_fetcher() == fetcher) {
      (*iter)->NotifyImageChanged(*bitmap);
      iter = requests_.erase(iter);
    } else {
      ++iter;
    }
  }

  if (!bitmap->isNull()) {
    CacheEntry* entry = new CacheEntry;
    entry->bitmap.reset(new SkBitmap(*bitmap));
    cache_.Put(fetcher->url(), entry);
  }

  RemoveFetcher(fetcher);
}
