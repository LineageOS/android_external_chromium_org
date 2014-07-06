// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PSL_MATCHING_HELPER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PSL_MATCHING_HELPER_H_

#include <string>

#include "base/basictypes.h"

class GURL;

namespace autofill {
struct PasswordForm;
}  // namespace autofill

namespace password_manager {

class PSLMatchingHelper {
 public:
  // Enum used for histogram tracking PSL Domain triggering.
  // New entries should only be added to the end of the enum (before *_COUNT) so
  // as to not disrupt existing data.
  enum PSLDomainMatchMetric {
    PSL_DOMAIN_MATCH_DISABLED = 0,
    PSL_DOMAIN_MATCH_NONE,
    PSL_DOMAIN_MATCH_FOUND,
    PSL_DOMAIN_MATCH_COUNT
  };

  PSLMatchingHelper();
  ~PSLMatchingHelper();

  bool IsMatchingEnabled() const;

  // Using the public suffix list for matching the origin is only needed for
  // websites that do not have a single hostname for entering credentials. It
  // would be better for their users if they did, but until then we help them
  // find
  // credentials across different hostnames. We know that accounts.google.com is
  // the only hostname we should be accepting credentials on for any domain
  // under
  // google.com, so we can apply a tighter policy for that domain.
  // For owners of domains where a single hostname is always used when your
  // users are entering their credentials, please contact palmer@chromium.org,
  // nyquist@chromium.org or file a bug at http://crbug.com/ to be added here.
  bool ShouldPSLDomainMatchingApply(
      const std::string& registry_controlled_domain) const;

  // Two URLs are considered a Public Suffix Domain match if they have the same
  // scheme, ports, and their registry controlled domains are equal. If one or
  // both arguments do not describe valid URLs, returns false.
  static bool IsPublicSuffixDomainMatch(const std::string& url1,
                                        const std::string& url2);

  // Two hosts are considered to belong to the same website when they share the
  // registry-controlled domain part.
  static std::string GetRegistryControlledDomain(const GURL& signon_realm);

  // This overrides both the command line flags and platform restrictions. This
  // function is not thread safe, and should be called before any other methods
  // of |PSLMatchingHelper| are called.
  static void EnablePublicSuffixDomainMatchingForTesting();

 private:
  static bool DeterminePSLEnabled();

  const bool psl_enabled_;

  // Default is false, once set to true, overrides |psl_enabled_|.
  static bool psl_enabled_override_;

  DISALLOW_COPY_AND_ASSIGN(PSLMatchingHelper);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PSL_MATCHING_HELPER_H_
