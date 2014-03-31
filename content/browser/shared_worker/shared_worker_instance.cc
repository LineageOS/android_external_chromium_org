// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_instance.h"

#include "base/logging.h"

namespace content {

SharedWorkerInstance::SharedWorkerInstance(
    const GURL& url,
    const base::string16& name,
    const base::string16& content_security_policy,
    blink::WebContentSecurityPolicyType security_policy_type,
    ResourceContext* resource_context,
    const WorkerStoragePartition& partition)
    : url_(url),
      name_(name),
      content_security_policy_(content_security_policy),
      security_policy_type_(security_policy_type),
      resource_context_(resource_context),
      partition_(partition) {
  DCHECK(resource_context_);
}

SharedWorkerInstance::~SharedWorkerInstance() {
}

bool SharedWorkerInstance::Matches(const GURL& match_url,
                                   const base::string16& match_name,
                                   const WorkerStoragePartition& partition,
    ResourceContext* resource_context) const {
  // ResourceContext equivalence is being used as a proxy to ensure we only
  // matched shared workers within the same BrowserContext.
  if (resource_context_ != resource_context)
    return false;

  // We must be in the same storage partition otherwise sharing will violate
  // isolation.
  if (!partition_.Equals(partition))
    return false;

  if (url_.GetOrigin() != match_url.GetOrigin())
    return false;

  if (name_.empty() && match_name.empty())
    return url_ == match_url;

  return name_ == match_name;
}

}  // namespace content
