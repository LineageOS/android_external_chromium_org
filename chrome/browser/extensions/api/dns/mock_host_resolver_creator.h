// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DNS_MOCK_HOST_RESOLVER_CREATOR_H_
#define CHROME_BROWSER_EXTENSIONS_API_DNS_MOCK_HOST_RESOLVER_CREATOR_H_
#pragma once

#include <string>

#include "base/memory/ref_counted.h"
#include "base/synchronization/waitable_event.h"

namespace net {
class MockHostResolver;
}

namespace extensions {

// Used only for testing. Creates a MockHostResolver, respecting threading
// constraints.
class MockHostResolverCreator
    : public base::RefCountedThreadSafe<MockHostResolverCreator> {
 public:
  static const std::string kHostname;
  static const std::string kAddress;

  MockHostResolverCreator();

  net::MockHostResolver* CreateMockHostResolver();
  void DeleteMockHostResolver();

 private:
  friend class base::RefCountedThreadSafe<MockHostResolverCreator>;
  virtual ~MockHostResolverCreator();

  void CreateMockHostResolverOnIOThread();
  void DeleteMockHostResolverOnIOThread();

  base::WaitableEvent resolver_event_;

  // The MockHostResolver asserts that it's used on the same thread on which
  // it's created, which is actually a stronger rule than its real counterpart.
  // But that's fine; it's good practice.
  //
  // Plain pointer because we have to manage lifetime manually.
  net::MockHostResolver* mock_host_resolver_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DNS_MOCK_HOST_RESOLVER_CREATOR_H_
