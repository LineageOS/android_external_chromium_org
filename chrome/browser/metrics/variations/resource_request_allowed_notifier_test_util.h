// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_VARIATIONS_RESOURCE_REQUEST_ALLOWED_NOTIFIER_TEST_UTIL_H_
#define CHROME_BROWSER_METRICS_VARIATIONS_RESOURCE_REQUEST_ALLOWED_NOTIFIER_TEST_UTIL_H_

#include "chrome/browser/metrics/variations/resource_request_allowed_notifier.h"

// A subclass of ResourceRequestAllowedNotifier used to expose some
// functionality for testing.
//
// By default, the constructor sets this class to override
// ResourceRequestsAllowed, so its state can be set with SetRequestsAllowed.
// This is meant for higher level tests of services to ensure they adhere to the
// notifications of the ResourceRequestAllowedNotifier. Lower level tests can
// disable this by calling SetRequestsAllowedOverride with the value they want
// it to return.
class TestRequestAllowedNotifier : public ResourceRequestAllowedNotifier {
 public:
  TestRequestAllowedNotifier();
  virtual ~TestRequestAllowedNotifier();

  // A version of |Init()| that accepts a custom EulaAcceptedNotifier.
  void InitWithEulaAcceptNotifier(
      Observer* observer,
      scoped_ptr<EulaAcceptedNotifier> eula_notifier);

  // Makes ResourceRequestsAllowed return |allowed| when it is called.
  void SetRequestsAllowedOverride(bool allowed);

  // Notify observers that requests are allowed. This will only work if
  // the observer is expecting a notification.
  void NotifyObserver();

  // ResourceRequestAllowedNotifier overrides:
  virtual bool ResourceRequestsAllowed() OVERRIDE;
  virtual EulaAcceptedNotifier* CreateEulaNotifier() OVERRIDE;

 private:
  scoped_ptr<EulaAcceptedNotifier> test_eula_notifier_;
  bool override_requests_allowed_;
  bool requests_allowed_;

  DISALLOW_COPY_AND_ASSIGN(TestRequestAllowedNotifier);
};

#endif  // CHROME_BROWSER_METRICS_VARIATIONS_RESOURCE_REQUEST_ALLOWED_NOTIFIER_TEST_UTIL_H_
