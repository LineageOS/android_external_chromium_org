// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_TOKEN_STORE_H_
#define CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_TOKEN_STORE_H_

#include <string>
#include "base/logging.h"
#include "base/threading/non_thread_safe.h"

// This class serves as the single repository for cloud print auth tokens. This
// is only used within the CloudPrintProxyCoreThread.

namespace cloud_print {

class CloudPrintTokenStore : public base::NonThreadSafe {
 public:
  // Returns the CloudPrintTokenStore instance for this thread. Will be NULL
  // if no instance was created in this thread before.
  static CloudPrintTokenStore* current();

  CloudPrintTokenStore();
  ~CloudPrintTokenStore();

  void SetToken(const std::string& token);
  std::string token() const {
    DCHECK(CalledOnValidThread());
    return token_;
  }

 private:
  std::string token_;

  DISALLOW_COPY_AND_ASSIGN(CloudPrintTokenStore);
};

}  // namespace cloud_print

#endif  // CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_TOKEN_STORE_H_
