// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/signin_manager.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/browser/net/gaia/token_service.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/notification_service.h"

const char kGetInfoEmailKey[] = "email";

const char kSyncOAuth2Scope[] = "https://www.googleapis.com/auth/chromesync";

SigninManager::SigninManager()
    : profile_(NULL), had_two_factor_error_(false) {}

SigninManager::~SigninManager() {}

// static
void SigninManager::RegisterUserPrefs(PrefService* user_prefs) {
  user_prefs->RegisterStringPref(prefs::kGoogleServicesUsername,
                                 "",
                                 PrefService::UNSYNCABLE_PREF);
}

void SigninManager::Initialize(Profile* profile) {
  profile_ = profile;
  username_ = profile_->GetPrefs()->GetString(prefs::kGoogleServicesUsername);
  profile_->GetTokenService()->Initialize(
      GaiaConstants::kChromeSource, profile_);
  if (!username_.empty()) {
    profile_->GetTokenService()->LoadTokensFromDB();
  }
}

bool SigninManager::IsInitialized() const {
  return profile_ != NULL;
}

void SigninManager::CleanupNotificationRegistration() {
#if !defined(OS_CHROMEOS)
  if (registrar_.IsRegistered(this,
                              chrome::NOTIFICATION_TOKEN_AVAILABLE,
                              NotificationService::AllSources())) {
    registrar_.Remove(this,
                      chrome::NOTIFICATION_TOKEN_AVAILABLE,
                      NotificationService::AllSources());
  }
#endif
}

// If a username already exists, the user is logged in.
const std::string& SigninManager::GetUsername() {
  return username_;
}

void SigninManager::SetUsername(const std::string& username) {
  username_ = username;
}

// static
void SigninManager::PrepareForSignin() {
  DCHECK(username_.empty());
#if !defined(OS_CHROMEOS)
  // The Sign out should clear the token service credentials.
  // Note: In CHROMEOS we might have valid credentials but still need to
  // set up 2-factor authentication.
  DCHECK(!profile_->GetTokenService()->AreCredentialsValid());
#endif
}

// Users must always sign out before they sign in again.
void SigninManager::StartOAuthSignIn() {
  PrepareForSignin();
  oauth_login_.reset(new GaiaOAuthFetcher(this,
                                          profile_->GetRequestContext(),
                                          profile_,
                                          GaiaConstants::kSyncService,
                                          kSyncOAuth2Scope));
  oauth_login_->StartGetOAuthToken();
}

// Users must always sign out before they sign in again.
void SigninManager::StartSignIn(const std::string& username,
                                const std::string& password,
                                const std::string& login_token,
                                const std::string& login_captcha) {
  PrepareForSignin();
  username_.assign(username);
  password_.assign(password);

  client_login_.reset(new GaiaAuthFetcher(this,
                                          GaiaConstants::kChromeSource,
                                          profile_->GetRequestContext()));
  client_login_->StartClientLogin(username,
                                  password,
                                  "",
                                  login_token,
                                  login_captcha,
                                  GaiaAuthFetcher::HostedAccountsNotAllowed);

  // Register for token availability.  The signin manager will pre-login the
  // user when the GAIA service token is ready for use.  Only do this if we
  // are not running in ChomiumOS, since it handles pre-login itself.
#if !defined(OS_CHROMEOS)
  if (profile_->GetPrefs()->GetBoolean(prefs::kAutologinEnabled)) {
    registrar_.Add(this,
                   chrome::NOTIFICATION_TOKEN_AVAILABLE,
                   NotificationService::AllSources());
  }
#endif
}

void SigninManager::ProvideSecondFactorAccessCode(
    const std::string& access_code) {
  DCHECK(!username_.empty() && !password_.empty() &&
      last_result_.data.empty());

  client_login_.reset(new GaiaAuthFetcher(this,
                                          GaiaConstants::kChromeSource,
                                          profile_->GetRequestContext()));
  client_login_->StartClientLogin(username_,
                                  access_code,
                                  "",
                                  std::string(),
                                  std::string(),
                                  GaiaAuthFetcher::HostedAccountsNotAllowed);
}

void SigninManager::SignOut() {
  if (!profile_)
    return;

  CleanupNotificationRegistration();

  client_login_.reset();
  last_result_ = ClientLoginResult();
  username_.clear();
  password_.clear();
  had_two_factor_error_ = false;
  profile_->GetPrefs()->SetString(prefs::kGoogleServicesUsername, username_);
  profile_->GetPrefs()->ScheduleSavePersistentPrefs();
  profile_->GetTokenService()->ResetCredentialsInMemory();
  profile_->GetTokenService()->EraseTokensFromDB();
}

void SigninManager::OnClientLoginSuccess(const ClientLoginResult& result) {
  last_result_ = result;
  // Make a request for the canonical email address.
  client_login_->StartGetUserInfo(result.lsid, kGetInfoEmailKey);
}

void SigninManager::OnGetUserInfoSuccess(const std::string& key,
                                         const std::string& value) {
  DCHECK(key == kGetInfoEmailKey);

  username_ = value;
  profile_->GetPrefs()->SetString(prefs::kGoogleServicesUsername, username_);
  profile_->GetPrefs()->ScheduleSavePersistentPrefs();

  GoogleServiceSigninSuccessDetails details(username_, password_);
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_GOOGLE_SIGNIN_SUCCESSFUL,
      Source<Profile>(profile_),
      Details<const GoogleServiceSigninSuccessDetails>(&details));

  password_.clear();  // Don't need it anymore.

  profile_->GetTokenService()->UpdateCredentials(last_result_);
  DCHECK(profile_->GetTokenService()->AreCredentialsValid());
  profile_->GetTokenService()->StartFetchingTokens();
}

void SigninManager::OnGetUserInfoKeyNotFound(const std::string& key) {
  DCHECK(key == kGetInfoEmailKey);
  LOG(ERROR) << "Account is not associated with a valid email address. "
             << "Login failed.";
  OnClientLoginFailure(GoogleServiceAuthError(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
}

void SigninManager::OnGetUserInfoFailure(const GoogleServiceAuthError& error) {
  LOG(ERROR) << "Unable to retreive the canonical email address. Login failed.";
  OnClientLoginFailure(error);
}

void SigninManager::OnTokenAuthFailure(const GoogleServiceAuthError& error) {
#if !defined(OS_CHROMEOS)
  VLOG(1) << "Unable to retrieve the token auth.";
  CleanupNotificationRegistration();
#endif
}

void SigninManager::OnClientLoginFailure(const GoogleServiceAuthError& error) {
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_GOOGLE_SIGNIN_FAILED,
      Source<Profile>(profile_),
      Details<const GoogleServiceAuthError>(&error));

  // We don't sign-out if the password was valid and we're just dealing with
  // a second factor error, and we don't sign out if we're dealing with
  // an invalid access code (again, because the password was valid).
  bool invalid_gaia = error.state() ==
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS;
  if (error.state() == GoogleServiceAuthError::TWO_FACTOR ||
      (had_two_factor_error_ && invalid_gaia)) {
    had_two_factor_error_ = true;
    return;
  }

  SignOut();
}

void SigninManager::OnGetOAuthTokenSuccess(const std::string& oauth_token) {
  VLOG(1) << "SigninManager::SigninManager::OnGetOAuthTokenSuccess";
}

void SigninManager::OnGetOAuthTokenFailure() {
  VLOG(1) << "SigninManager::OnGetOAuthTokenFailure";
}

void SigninManager::OnOAuthGetAccessTokenSuccess(const std::string& token,
                                                 const std::string& secret) {
  VLOG(1) << "SigninManager::OnOAuthGetAccessTokenSuccess";
}

void SigninManager::OnOAuthGetAccessTokenFailure(
    const GoogleServiceAuthError& error) {
  VLOG(1) << "SigninManager::OnOAuthGetAccessTokenFailure";
}

void SigninManager::OnOAuthWrapBridgeSuccess(const std::string& service_name,
                                             const std::string& token,
                                             const std::string& expires_in) {
  VLOG(1) << "SigninManager::OnOAuthWrapBridgeSuccess";
}

void SigninManager::OnOAuthWrapBridgeFailure(
    const GoogleServiceAuthError& error) {
  VLOG(1) << "SigninManager::OnOAuthWrapBridgeFailure";
}

void SigninManager::OnUserInfoSuccess(const std::string& email) {
  VLOG(1) << "SigninManager::OnUserInfoSuccess(\"" << email << "\")";
}

void SigninManager::OnUserInfoFailure(const GoogleServiceAuthError& error) {
  VLOG(1) << "SigninManager::OnUserInfoFailure";
}

void SigninManager::Observe(int type,
                            const NotificationSource& source,
                            const NotificationDetails& details) {
#if !defined(OS_CHROMEOS)
  DCHECK(type == chrome::NOTIFICATION_TOKEN_AVAILABLE);
  TokenService::TokenAvailableDetails* tok_details =
      Details<TokenService::TokenAvailableDetails>(details).ptr();

  // If a GAIA service token has become available, use it to pre-login the
  // user to other services that depend on GAIA credentials.
  if (tok_details->service() == GaiaConstants::kGaiaService) {
    if (client_login_.get() == NULL) {
      client_login_.reset(new GaiaAuthFetcher(this,
                                              GaiaConstants::kChromeSource,
                                              profile_->GetRequestContext()));
    }

    client_login_->StartTokenAuth(tok_details->token());

    // We only want to do this once per sign-in.
    CleanupNotificationRegistration();
  }
#endif
}
