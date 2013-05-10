// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_FACTORY_H_

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace extensions {

class TabCaptureRegistry;

class TabCaptureRegistryFactory : public ProfileKeyedServiceFactory {
 public:
   static TabCaptureRegistry* GetForProfile(Profile* profile);

   static TabCaptureRegistryFactory* GetInstance();

  // ProfileKeyedBaseFactory:
  virtual bool ServiceIsCreatedWithProfile() const OVERRIDE;

 private:
  friend struct DefaultSingletonTraits<TabCaptureRegistryFactory>;

  TabCaptureRegistryFactory();
  virtual ~TabCaptureRegistryFactory();

  // ProfileKeyedServiceFactory:
  virtual ProfileKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const OVERRIDE;
  virtual content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(TabCaptureRegistryFactory);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_FACTORY_H_
