// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_FACTORY_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

template <typename T> struct DefaultSingletonTraits;

class PrefRegistrySimple;
class Profile;

namespace policy {

class PolicyCertService;
class PolicyCertVerifier;

// Factory to create PolicyCertServices.
class PolicyCertServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns an existing PolicyCertService for |profile|. See
  // CreateForProfile.
  static PolicyCertService* GetForProfile(Profile* profile);

  // Creates a new PolicyCertService and returns the associated
  // PolicyCertVerifier. Returns NULL if this service isn't allowed for
  // |profile|, i.e. if NetworkConfigurationUpdater doesn't exist.
  // This service is created separately for the original profile and the
  // incognito profile.
  // Note: NetworkConfigurationUpdater is currently only created for the primary
  // user's profile.
  static scoped_ptr<PolicyCertVerifier> CreateForProfile(Profile* profile);

  static PolicyCertServiceFactory* GetInstance();

  // Used to mark or clear |user_id| as having used certificates pushed by
  // policy before.
  static void SetUsedPolicyCertificates(const std::string& user_id);
  static void ClearUsedPolicyCertificates(const std::string& user_id);
  static bool UsedPolicyCertificates(const std::string& user_id);

  static void RegisterPrefs(PrefRegistrySimple* local_state);

 private:
  friend struct DefaultSingletonTraits<PolicyCertServiceFactory>;

  PolicyCertServiceFactory();
  virtual ~PolicyCertServiceFactory();

  // BrowserContextKeyedServiceFactory:
  virtual KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const OVERRIDE;
  virtual content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const OVERRIDE;
  virtual void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) OVERRIDE;
  virtual bool ServiceIsNULLWhileTesting() const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(PolicyCertServiceFactory);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_FACTORY_H_
