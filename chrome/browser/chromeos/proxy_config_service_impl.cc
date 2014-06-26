// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/proxy_config_service_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/users/user_manager.h"
#include "chrome/browser/chromeos/net/proxy_config_handler.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/prefs/proxy_config_dictionary.h"
#include "chrome/browser/prefs/proxy_prefs.h"
#include "chrome/common/pref_names.h"
#include "chromeos/network/network_profile.h"
#include "chromeos/network/network_profile_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

namespace chromeos {

namespace {

// Writes the proxy config of |network| to |proxy_config|.  Set |onc_source| to
// the source of this configuration. Returns false if no
// proxy was configured for this network.
bool GetProxyConfig(const PrefService* profile_prefs,
                    const PrefService* local_state_prefs,
                    const NetworkState& network,
                    net::ProxyConfig* proxy_config,
                    ::onc::ONCSource* onc_source) {
  scoped_ptr<ProxyConfigDictionary> proxy_dict =
      proxy_config::GetProxyConfigForNetwork(
          profile_prefs, local_state_prefs, network, onc_source);
  if (!proxy_dict)
    return false;
  return PrefProxyConfigTrackerImpl::PrefConfigToNetConfig(*proxy_dict,
                                                           proxy_config);
}

}  // namespace

ProxyConfigServiceImpl::ProxyConfigServiceImpl(PrefService* profile_prefs,
                                               PrefService* local_state_prefs)
    : PrefProxyConfigTrackerImpl(profile_prefs ? profile_prefs
                                               : local_state_prefs),
      active_config_state_(ProxyPrefs::CONFIG_UNSET),
      profile_prefs_(profile_prefs),
      local_state_prefs_(local_state_prefs),
      pointer_factory_(this) {
  const base::Closure proxy_change_callback = base::Bind(
      &ProxyConfigServiceImpl::OnProxyPrefChanged, base::Unretained(this));

  if (profile_prefs) {
    profile_pref_registrar_.Init(profile_prefs);
    profile_pref_registrar_.Add(prefs::kOpenNetworkConfiguration,
                                proxy_change_callback);
    profile_pref_registrar_.Add(prefs::kUseSharedProxies,
                                proxy_change_callback);
  }
  local_state_pref_registrar_.Init(local_state_prefs);
  local_state_pref_registrar_.Add(prefs::kDeviceOpenNetworkConfiguration,
                                  proxy_change_callback);

  // Register for changes to the default network.
  NetworkStateHandler* state_handler =
      NetworkHandler::Get()->network_state_handler();
  state_handler->AddObserver(this, FROM_HERE);
  DefaultNetworkChanged(state_handler->DefaultNetwork());
}

ProxyConfigServiceImpl::~ProxyConfigServiceImpl() {
  if (NetworkHandler::IsInitialized()) {
    NetworkHandler::Get()->network_state_handler()->RemoveObserver(
        this, FROM_HERE);
  }
}

void ProxyConfigServiceImpl::OnProxyConfigChanged(
    ProxyPrefs::ConfigState config_state,
    const net::ProxyConfig& config) {
  VLOG(1) << "Got prefs change: "
          << ProxyPrefs::ConfigStateToDebugString(config_state)
          << ", mode=" << config.proxy_rules().type;
  DetermineEffectiveConfigFromDefaultNetwork();
}

void ProxyConfigServiceImpl::OnProxyPrefChanged() {
  DetermineEffectiveConfigFromDefaultNetwork();
}

void ProxyConfigServiceImpl::DefaultNetworkChanged(
    const NetworkState* new_network) {
  std::string new_network_path;
  if (new_network)
    new_network_path = new_network->path();

  VLOG(1) << "DefaultNetworkChanged to '" << new_network_path << "'.";
  VLOG_IF(1, new_network) << "New network: name=" << new_network->name()
                          << ", profile=" << new_network->profile_path();

  // Even if the default network is the same, its proxy config (e.g. if private
  // version of network replaces the shared version after login), or
  // use-shared-proxies setting (e.g. after login) may have changed, so
  // re-determine effective proxy config, and activate if different.
  DetermineEffectiveConfigFromDefaultNetwork();
}

// static
bool ProxyConfigServiceImpl::IgnoreProxy(const PrefService* profile_prefs,
                                         const std::string network_profile_path,
                                         ::onc::ONCSource onc_source) {
  if (!profile_prefs) {
    // If the profile preference are not available, this must be the object
    // associated to local state used for system requests or login-profile. Make
    // sure that proxies are enabled.
    VLOG(1) << "Use proxy for system requests and sign-in screen.";
    return false;
  }

  if (network_profile_path.empty())
    return true;

  const NetworkProfile* profile = NetworkHandler::Get()
      ->network_profile_handler()->GetProfileForPath(network_profile_path);
  if (!profile) {
    VLOG(1) << "Unknown profile_path '" << network_profile_path
            << "'. Ignoring proxy.";
    return true;
  }
  if (profile->type() == NetworkProfile::TYPE_USER) {
    VLOG(1) << "Respect proxy of not-shared networks.";
    return false;
  }
  if (onc_source == ::onc::ONC_SOURCE_DEVICE_POLICY) {
    policy::BrowserPolicyConnectorChromeOS* connector =
        g_browser_process->platform_part()->browser_policy_connector_chromeos();
    const User* logged_in_user = UserManager::Get()->GetLoggedInUser();
    if (connector->GetUserAffiliation(logged_in_user->email()) ==
        policy::USER_AFFILIATION_MANAGED) {
      VLOG(1) << "Respecting proxy for network, as logged-in user belongs to "
              << "the domain the device is enrolled to.";
      return false;
    }
  }

  // This network is shared and not managed by the user's domain.
  bool use_shared_proxies = profile_prefs->GetBoolean(prefs::kUseSharedProxies);
  VLOG(1) << "Use proxy of shared network: " << use_shared_proxies;
  return !use_shared_proxies;
}

void ProxyConfigServiceImpl::DetermineEffectiveConfigFromDefaultNetwork() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  const NetworkState* network = handler->DefaultNetwork();

  // Get prefs proxy config if available.
  net::ProxyConfig pref_config;
  ProxyPrefs::ConfigState pref_state = GetProxyConfig(&pref_config);

  // Get network proxy config if available.
  net::ProxyConfig network_config;
  net::ProxyConfigService::ConfigAvailability network_availability =
      net::ProxyConfigService::CONFIG_UNSET;
  bool ignore_proxy = true;
  if (network) {
    ::onc::ONCSource onc_source = ::onc::ONC_SOURCE_NONE;
    const bool network_proxy_configured = chromeos::GetProxyConfig(
        prefs(), local_state_prefs_, *network, &network_config, &onc_source);
    ignore_proxy =
        IgnoreProxy(profile_prefs_, network->profile_path(), onc_source);

    // If network is shared but use-shared-proxies is off, use direct mode.
    if (ignore_proxy) {
      network_config = net::ProxyConfig();
      network_availability = net::ProxyConfigService::CONFIG_VALID;
    } else if (network_proxy_configured) {
      // Network is private or shared with user using shared proxies.
      VLOG(1) << this << ": using proxy of network " << network->path();
      network_availability = net::ProxyConfigService::CONFIG_VALID;
    }
  }

  // Determine effective proxy config, either from prefs or network.
  ProxyPrefs::ConfigState effective_config_state;
  net::ProxyConfig effective_config;
  GetEffectiveProxyConfig(pref_state, pref_config,
                          network_availability, network_config, ignore_proxy,
                          &effective_config_state, &effective_config);

  // Activate effective proxy and store into |active_config_|.
  // If last update didn't complete, we definitely update now.
  bool update_now = update_pending();
  if (!update_now) {  // Otherwise, only update now if there're changes.
    update_now = active_config_state_ != effective_config_state ||
                 (active_config_state_ != ProxyPrefs::CONFIG_UNSET &&
                  !active_config_.Equals(effective_config));
  }
  if (update_now) {  // Activate and store new effective config.
    active_config_state_ = effective_config_state;
    if (active_config_state_ != ProxyPrefs::CONFIG_UNSET)
      active_config_ = effective_config;
    // If effective config is from system (i.e. network), it's considered a
    // special kind of prefs that ranks below policy/extension but above
    // others, so bump it up to CONFIG_OTHER_PRECEDE to force its precedence
    // when PrefProxyConfigTrackerImpl pushes it to ChromeProxyConfigService.
    if (effective_config_state == ProxyPrefs::CONFIG_SYSTEM)
      effective_config_state = ProxyPrefs::CONFIG_OTHER_PRECEDE;
    // If config is manual, add rule to bypass local host.
    if (effective_config.proxy_rules().type !=
        net::ProxyConfig::ProxyRules::TYPE_NO_RULES) {
      effective_config.proxy_rules().bypass_rules.AddRuleToBypassLocal();
    }
    PrefProxyConfigTrackerImpl::OnProxyConfigChanged(effective_config_state,
                                                     effective_config);
    if (VLOG_IS_ON(1) && !update_pending()) {  // Update was successful.
      scoped_ptr<base::DictionaryValue> config_dict(effective_config.ToValue());
      VLOG(1) << this << ": Proxy changed: "
              << ProxyPrefs::ConfigStateToDebugString(active_config_state_)
              << ", " << *config_dict;
    }
  }
}

}  // namespace chromeos
