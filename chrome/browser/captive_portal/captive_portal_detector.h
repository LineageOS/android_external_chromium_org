// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_DETECTOR_H_
#define CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_DETECTOR_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "base/time.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

class GURL;

namespace net {
class URLFetcher;
}

namespace captive_portal {

// Possible results of an attempt to detect a captive portal.
enum Result {
  // There's a confirmed connection to the Internet.
  RESULT_INTERNET_CONNECTED,
  // The URL request received a network or HTTP error, or a non-HTTP response.
  RESULT_NO_RESPONSE,
  // The URL request apparently encountered a captive portal.  It received a
  // a valid HTTP response with a 2xx other than 204, 3xx, or 511 status code.
  RESULT_BEHIND_CAPTIVE_PORTAL,
  RESULT_COUNT
};

class CaptivePortalDetector : public net::URLFetcherDelegate,
                              public base::NonThreadSafe {
 public:
  struct Results {
    Results() : result(RESULT_NO_RESPONSE) {
    }

    Result result;
    base::TimeDelta retry_after_delta;
  };

  typedef base::Callback<void(const Results& results)> DetectionCallback;

  // The test URL.  When connected to the Internet, it should return a
  // blank page with a 204 status code.  When behind a captive portal,
  // requests for this URL should get an HTTP redirect or a login
  // page.  When neither is true, no server should respond to requests
  // for this URL.
  static const char kDefaultURL[];

  explicit CaptivePortalDetector(
      const scoped_refptr<net::URLRequestContextGetter>& request_context);
  virtual ~CaptivePortalDetector();

  // Triggers a check for a captive portal. After completion, runs the
  // |callback|.
  void DetectCaptivePortal(const GURL& url, const DetectionCallback& callback);

  // Cancels captive portal check.
  void Cancel();

 private:
  friend class CaptivePortalDetectorTestBase;

  // net::URLFetcherDelegate:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

  // Takes a net::URLFetcher that has finished trying to retrieve the
  // test URL, and fills a Results struct based on its result.  If the
  // response is a 503 with a Retry-After header, |retry_after| field
  // of |results| is populated accordingly.  Otherwise, it's set to
  // base::TimeDelta().
  void GetCaptivePortalResultFromResponse(const net::URLFetcher* url_fetcher,
                                          Results* results) const;

  // Returns the current time. Used only when determining time until a
  // Retry-After date.
  base::Time GetCurrentTime() const;

  // Returns true if a captive portal check is currently running.
  bool FetchingURL() const;

  // Sets current test time. Used by unit tests.
  void set_time_for_testing(const base::Time& time) {
    time_for_testing_ = time;
  }

  // Advances current test time. Used by unit tests.
  void advance_time_for_testing(const base::TimeDelta& delta) {
    time_for_testing_ += delta;
  }

  // URL request context.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DetectionCallback detection_callback_;

  scoped_ptr<net::URLFetcher> url_fetcher_;

  // Test time used by unit tests.
  base::Time time_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalDetector);
};

}  // namespace captive_portal

#endif  // CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_DETECTOR_H_
