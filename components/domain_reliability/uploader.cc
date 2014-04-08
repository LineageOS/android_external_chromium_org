// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/domain_reliability/uploader.h"

#include "base/bind.h"
#include "base/metrics/sparse_histogram.h"
#include "base/stl_util.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

namespace domain_reliability {

namespace {

class DomainReliabilityUploaderImpl
    : public DomainReliabilityUploader, net::URLFetcherDelegate {
 public:
  DomainReliabilityUploaderImpl(
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter)
      : url_request_context_getter_(url_request_context_getter) {}

  virtual ~DomainReliabilityUploaderImpl() {
    // Delete any in-flight URLFetchers.
    STLDeleteContainerPairFirstPointers(
        upload_callbacks_.begin(), upload_callbacks_.end());
  }

  // DomainReliabilityUploader implementation:
  virtual void UploadReport(
      const std::string& report_json,
      const GURL& upload_url,
      const DomainReliabilityUploader::UploadCallback& callback) OVERRIDE {
    VLOG(1) << "Uploading report to " << upload_url;
    VLOG(2) << "Report JSON: " << report_json;

    net::URLFetcher* fetcher =
        net::URLFetcher::Create(0, upload_url, net::URLFetcher::POST, this);
    fetcher->SetRequestContext(url_request_context_getter_);
    fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                          net::LOAD_DO_NOT_SAVE_COOKIES);
    fetcher->SetUploadData("text/json", report_json);
    fetcher->SetAutomaticallyRetryOn5xx(false);
    fetcher->Start();

    upload_callbacks_[fetcher] = callback;
  }

  // net::URLFetcherDelegate implementation:
  virtual void OnURLFetchComplete(
      const net::URLFetcher* fetcher) OVERRIDE {
    DCHECK(fetcher);

    UploadCallbackMap::iterator callback_it = upload_callbacks_.find(fetcher);
    DCHECK(callback_it != upload_callbacks_.end());

    VLOG(1) << "Upload finished with " << fetcher->GetResponseCode();

    UMA_HISTOGRAM_SPARSE_SLOWLY("DomainReliability.UploadResponseCode",
                                fetcher->GetResponseCode());

    bool success = fetcher->GetResponseCode() == 200;
    callback_it->second.Run(success);

    delete callback_it->first;
    upload_callbacks_.erase(callback_it);
  }

 private:
  using DomainReliabilityUploader::UploadCallback;
  typedef std::map<const net::URLFetcher*, UploadCallback> UploadCallbackMap;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  UploadCallbackMap upload_callbacks_;
};

}  // namespace

DomainReliabilityUploader::DomainReliabilityUploader() {}

DomainReliabilityUploader::~DomainReliabilityUploader() {}

// static
scoped_ptr<DomainReliabilityUploader> DomainReliabilityUploader::Create(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter) {
  return scoped_ptr<DomainReliabilityUploader>(
      new DomainReliabilityUploaderImpl(url_request_context_getter));
}

}  // namespace domain_reliability
