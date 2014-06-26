// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOMAIN_RELIABILITY_SERVICE_H_
#define COMPONENTS_DOMAIN_RELIABILITY_SERVICE_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/sequenced_task_runner.h"
#include "components/domain_reliability/clear_mode.h"
#include "components/domain_reliability/domain_reliability_export.h"
#include "components/keyed_service/core/keyed_service.h"

namespace net {
class URLRequestContextGetter;
};

namespace domain_reliability {

class DomainReliabilityMonitor;

// DomainReliabilityService is a KeyedService that manages a Monitor that lives
// on another thread (as provided by the URLRequestContextGetter's task runner)
// and proxies (selected) method calls to it. Destruction of the Monitor (on
// that thread) is the responsibility of the caller.
class DOMAIN_RELIABILITY_EXPORT DomainReliabilityService
    : public KeyedService {
 public:
  // Creates a DomainReliabilityService that will contain a Monitor with the
  // given upload reporter string.
  static DomainReliabilityService* Create(
      const std::string& upload_reporter_string);

  virtual ~DomainReliabilityService();

  // Initializes the Service: given the task runner on which Monitor methods
  // should be called, creates the Monitor and returns it. Can be called at
  // most once, and must be called before any of the below methods can be
  // called. The caller is responsible for destroying the Monitor on the given
  // task runner when it is no longer needed.
  virtual scoped_ptr<DomainReliabilityMonitor> CreateMonitor(
      scoped_refptr<base::SequencedTaskRunner> network_task_runner) = 0;

  // Clears browsing data on the associated Monitor. |Init()| must have been
  // called first.
  virtual void ClearBrowsingData(DomainReliabilityClearMode clear_mode,
                                 const base::Closure& callback) = 0;

 protected:
  DomainReliabilityService();

 private:
  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityService);
};

}  // namespace domain_reliability

#endif  // COMPONENTS_DOMAIN_RELIABILITY_SERVICE_H_
