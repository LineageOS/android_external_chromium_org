// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/browser_context_keyed_service/browser_context_keyed_service_factory.h"

#if defined(OS_ANDROID)
#include "chrome/browser/signin/android_profile_oauth2_token_service.h"
#endif

class ProfileOAuth2TokenService;
class Profile;

// Singleton that owns all ProfileOAuth2TokenServices and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated ProfileOAuth2TokenService.
class ProfileOAuth2TokenServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of ProfileOAuth2TokenService associated with this
  // profile (creating one if none exists). Returns NULL if this profile
  // cannot have a ProfileOAuth2TokenService (for example, if |profile| is
  // incognito).  On Android, returns the AndroidProfileOAuth2TokenService
  // specialization.
#if defined(OS_ANDROID)
  static AndroidProfileOAuth2TokenService* GetForProfile(Profile* profile);
#else
  static ProfileOAuth2TokenService* GetForProfile(Profile* profile);
#endif

  // Returns an instance of the ProfileOAuth2TokenServiceFactory singleton.
  static ProfileOAuth2TokenServiceFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<ProfileOAuth2TokenServiceFactory>;

  ProfileOAuth2TokenServiceFactory();
  virtual ~ProfileOAuth2TokenServiceFactory();

  // BrowserContextKeyedServiceFactory implementation.
  virtual BrowserContextKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(ProfileOAuth2TokenServiceFactory);
};

#endif  // CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_FACTORY_H_
