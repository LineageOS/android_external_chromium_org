// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_URL_REQUEST_JOB_FACTORY_H_
#define NET_URL_REQUEST_URL_REQUEST_JOB_FACTORY_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/threading/non_thread_safe.h"
#include "net/base/net_export.h"

class GURL;

namespace net {

class NetworkDelegate;
class URLRequest;
class URLRequestJob;

class NET_EXPORT URLRequestJobFactory
    : NON_EXPORTED_BASE(public base::NonThreadSafe) {
 public:
  // TODO(shalev): Move this to URLRequestJobFactoryImpl.
  class NET_EXPORT ProtocolHandler {
   public:
    virtual ~ProtocolHandler();

    virtual URLRequestJob* MaybeCreateJob(
        URLRequest* request, NetworkDelegate* network_delegate) const = 0;
  };

  URLRequestJobFactory();
  virtual ~URLRequestJobFactory();

  virtual URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      URLRequest* request,
      NetworkDelegate* network_delegate) const = 0;

  virtual bool IsHandledProtocol(const std::string& scheme) const = 0;

  virtual bool IsHandledURL(const GURL& url) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestJobFactory);
};

}  // namespace net

#endif  // NET_URL_REQUEST_URL_REQUEST_JOB_FACTORY_H_
