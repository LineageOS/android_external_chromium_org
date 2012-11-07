// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/captive_portal/captive_portal_detector.h"

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/message_loop.h"
#include "base/time.h"
#include "chrome/browser/captive_portal/testing_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "googleurl/src/gurl.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace captive_portal {

namespace {

class CaptivePortalClient {
 public:
  explicit CaptivePortalClient(CaptivePortalDetector* captive_portal_detector)
      : captive_portal_detector_(captive_portal_detector),
        num_results_received_(0) {
  }

  void OnPortalDetectionCompleted(
      const CaptivePortalDetector::Results& results) {
    results_ = results;
    ++num_results_received_;
  }

  const CaptivePortalDetector::Results& captive_portal_results() const {
    return results_;
  }

  int num_results_received() const { return num_results_received_; }

 private:
  CaptivePortalDetector* captive_portal_detector_;

  CaptivePortalDetector::Results results_;
  int num_results_received_;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalClient);
};

}  // namespace

class CaptivePortalDetectorTest : public testing::Test,
                                  public CaptivePortalDetectorTestBase {
 public:
  CaptivePortalDetectorTest() : detector_(profile_.GetRequestContext()) {
    set_detector(&detector_);
  }

  virtual ~CaptivePortalDetectorTest() {}

  void RunTest(const CaptivePortalDetector::Results& expected_results,
               int net_error,
               int status_code,
               const char* response_headers) {
    ASSERT_FALSE(FetchingURL());

    GURL url(CaptivePortalDetector::kDefaultURL);
    CaptivePortalClient client(detector());

    net::TestURLFetcherFactory factory;
    detector()->DetectCaptivePortal(url,
        base::Bind(&CaptivePortalClient::OnPortalDetectionCompleted,
                   base::Unretained(&client)));

    ASSERT_TRUE(FetchingURL());
    MessageLoop::current()->RunUntilIdle();

    net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
    if (net_error != net::OK) {
      EXPECT_FALSE(response_headers);
      fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                                net_error));
    } else {
      fetcher->set_response_code(status_code);
      if (response_headers) {
        scoped_refptr<net::HttpResponseHeaders> headers(
            CreateResponseHeaders(response_headers));
        EXPECT_EQ(status_code, headers->response_code());
        fetcher->set_response_headers(headers);
      }
    }

    OnURLFetchComplete(fetcher);

    EXPECT_FALSE(FetchingURL());
    EXPECT_EQ(1, client.num_results_received());
    EXPECT_EQ(expected_results.result, client.captive_portal_results().result);
    EXPECT_EQ(expected_results.retry_after_delta,
              client.captive_portal_results().retry_after_delta);
  }

  void RunCancelTest() {
    ASSERT_FALSE(FetchingURL());

    GURL url(CaptivePortalDetector::kDefaultURL);
    CaptivePortalClient client(detector());

    net::TestURLFetcherFactory factory;
    detector()->DetectCaptivePortal(url,
        base::Bind(&CaptivePortalClient::OnPortalDetectionCompleted,
                   base::Unretained(&client)));

    ASSERT_TRUE(FetchingURL());
    MessageLoop::current()->RunUntilIdle();

    detector()->Cancel();

    ASSERT_FALSE(FetchingURL());
    EXPECT_EQ(0, client.num_results_received());
  }

 private:
  MessageLoop message_loop_;

  // Definition order does matter.
  TestingProfile profile_;
  CaptivePortalDetector detector_;
};

// Test that the CaptivePortalDetector returns the expected result
// codes in response to a variety of probe results.
TEST_F(CaptivePortalDetectorTest, CaptivePortalResultCodes) {
  CaptivePortalDetector::Results results;
  results.result = RESULT_INTERNET_CONNECTED;

  RunTest(results, net::OK, 204, NULL);

  // The server may return an HTTP error when it's acting up.
  results.result = RESULT_NO_RESPONSE;
  RunTest(results, net::OK, 500, NULL);

  // Generic network error case.
  results.result = RESULT_NO_RESPONSE;
  RunTest(results, net::ERR_TIMED_OUT, -1, NULL);

  // In the general captive portal case, the portal will return a page with a
  // 200 status.
  results.result = RESULT_BEHIND_CAPTIVE_PORTAL;
  RunTest(results, net::OK, 200, NULL);

  // Some captive portals return 511 instead, to advertise their captive
  // portal-ness.
  results.result = RESULT_BEHIND_CAPTIVE_PORTAL;
  RunTest(results, net::OK, 511, NULL);
}

// Check a Retry-After header that contains a delay in seconds.
TEST_F(CaptivePortalDetectorTest, CaptivePortalRetryAfterSeconds) {
  const char* retry_after = "HTTP/1.1 503 OK\nRetry-After: 101\n\n";
  CaptivePortalDetector::Results results;

  // Check that Retry-After headers work both on the first request to return a
  // result and on subsequent requests.
  results.result = RESULT_NO_RESPONSE;
  results.retry_after_delta = base::TimeDelta::FromSeconds(101);
  RunTest(results, net::OK, 503, retry_after);

  results.result = RESULT_INTERNET_CONNECTED;
  results.retry_after_delta = base::TimeDelta();
  RunTest(results, net::OK, 204, NULL);
}

// Check a Retry-After header that contains a date.
TEST_F(CaptivePortalDetectorTest, CaptivePortalRetryAfterDate) {
  const char* retry_after =
      "HTTP/1.1 503 OK\nRetry-After: Tue, 17 Apr 2012 18:02:51 GMT\n\n";
  CaptivePortalDetector::Results results;

  // base has a function to get a time in the right format from a string, but
  // not the other way around.
  base::Time start_time;
  ASSERT_TRUE(base::Time::FromString("Tue, 17 Apr 2012 18:02:00 GMT",
                                     &start_time));
  base::Time retry_after_time;
  ASSERT_TRUE(base::Time::FromString("Tue, 17 Apr 2012 18:02:51 GMT",
                                     &retry_after_time));

  SetTime(start_time);

  results.result = RESULT_NO_RESPONSE;
  results.retry_after_delta = retry_after_time - start_time;
  RunTest(results, net::OK, 503, retry_after);
}

// Check invalid Retry-After headers are ignored.
TEST_F(CaptivePortalDetectorTest, CaptivePortalRetryAfterInvalid) {
  const char* retry_after = "HTTP/1.1 503 OK\nRetry-After: Christmas\n\n";
  CaptivePortalDetector::Results results;

  results.result = RESULT_NO_RESPONSE;
  RunTest(results, net::OK, 503, retry_after);
}

TEST_F(CaptivePortalDetectorTest, Cancel) {
  RunCancelTest();
  CaptivePortalDetector::Results results;
  results.result = RESULT_INTERNET_CONNECTED;
  RunTest(results, net::OK, 204, NULL);
}

}  // namespace captive_portal
