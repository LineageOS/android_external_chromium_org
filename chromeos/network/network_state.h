// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_STATE_H_
#define CHROMEOS_NETWORK_NETWORK_STATE_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "chromeos/network/managed_state.h"
#include "chromeos/network/onc/onc_constants.h"

namespace chromeos {

// Simple class to provide network state information about a network service.
// This class should always be passed as a const* and should never be held
// on to. Store network_state->path() (defined in ManagedState) instead and
// call NetworkStateHandler::GetNetworkState(path) to retrieve the state for
// the network.
class CHROMEOS_EXPORT NetworkState : public ManagedState {
public:
  typedef std::vector<int> FrequencyList;

  explicit NetworkState(const std::string& path);
  virtual ~NetworkState();

  // ManagedState overrides
  // If you change this method, update GetProperties too.
  virtual bool PropertyChanged(const std::string& key,
                               const base::Value& value) OVERRIDE;
  virtual void InitialPropertiesReceived() OVERRIDE;

  // Fills |dictionary| with the state properties. All the properties that are
  // accepted by PropertyChanged are stored in |dictionary|, no other values are
  // stored.
  void GetProperties(base::DictionaryValue* dictionary) const;

  // Fills |dictionary| with the state properties required to configure a
  // network.
  void GetConfigProperties(base::DictionaryValue* dictionary) const;

  // Accessors
  const std::string& security() const { return security_; }
  const std::string& ip_address() const { return ip_address_; }
  const std::vector<std::string>& dns_servers() const { return dns_servers_; }
  const std::string& device_path() const { return device_path_; }
  const std::string& guid() const { return guid_; }
  const std::string& connection_state() const { return connection_state_; }
  const std::string& profile_path() const { return profile_path_; }
  const std::string& error() const { return error_; }
  const std::string& error_details() const { return error_details_; }
  bool auto_connect() const { return auto_connect_; }
  bool favorite() const { return favorite_; }
  int priority() const { return priority_; }
  const base::DictionaryValue& proxy_config() const { return proxy_config_; }
  onc::ONCSource onc_source() const { return onc_source_; }
  // Wireless property accessors
  int signal_strength() const { return signal_strength_; }
  bool connectable() const { return connectable_; }
  // Wifi property accessors
  bool passphrase_required() const { return passphrase_required_; }
  const FrequencyList& wifi_frequencies() const { return wifi_frequencies_; }
  // Cellular property accessors
  const std::string& technology() const { return technology_; }
  const std::string& activation_state() const { return activation_state_; }
  const std::string& roaming() const { return roaming_; }
  bool activate_over_non_cellular_networks() const {
    return activate_over_non_cellular_networks_;
  }
  bool cellular_out_of_credits() const { return cellular_out_of_credits_; }
  const std::string& usage_url() const { return usage_url_; }
  const std::string& payment_url() const { return payment_url_; }
  const std::string& post_method() const { return post_method_; }
  const std::string& post_data() const { return post_data_; }

  bool IsConnectedState() const;
  bool IsConnectingState() const;

  // Returns true if |error_| contains an authentication error.
  bool HasAuthenticationError() const;

  // Helpers (used e.g. when a state is cached)
  static bool StateIsConnected(const std::string& connection_state);
  static bool StateIsConnecting(const std::string& connection_state);

  // Helper to return a full prefixed version of an IPConfig property
  // key.
  static std::string IPConfigProperty(const char* key);

 private:
  friend class NetworkStateHandler;
  friend class NetworkChangeNotifierChromeosUpdateTest;

  // Updates the name from hex_ssid_ if provided, and validates name_.
  void UpdateName();

  // Called by NetworkStateHandler when the ip config changes.
  void set_ip_address(const std::string& ip_address) {
    ip_address_ = ip_address;
  }
  void set_dns_servers(const std::vector<std::string>& dns_servers) {
    dns_servers_ = dns_servers;
  }

  // TODO(gauravsh): Audit the list of properties that we are caching. We should
  // only be doing this for commonly accessed properties. crbug.com/252553
  // Common Network Service properties
  std::string security_;
  std::string device_path_;
  std::string guid_;
  std::string connection_state_;
  std::string profile_path_;
  std::string error_;
  std::string error_details_;
  bool auto_connect_;
  bool favorite_;
  int priority_;
  // TODO(pneubeck): Remove ProxyConfig and ONCSource once
  // NetworkConfigurationHandler provides proxy configuration. crbug/241775
  base::DictionaryValue proxy_config_;
  onc::ONCSource onc_source_;
  // IPConfig properties.
  // Note: These do not correspond to actual Shill.Service properties
  // but are derived from the service's corresponding IPConfig object.
  std::string ip_address_;
  std::vector<std::string> dns_servers_;
  // Wireless properties
  int signal_strength_;
  bool connectable_;
  // Wifi properties
  std::string hex_ssid_;
  std::string country_code_;
  bool passphrase_required_;
  FrequencyList wifi_frequencies_;
  // Cellular properties
  std::string technology_;
  std::string activation_state_;
  std::string roaming_;
  bool activate_over_non_cellular_networks_;
  bool cellular_out_of_credits_;
  // Cellular payment portal properties.
  std::string usage_url_;
  std::string payment_url_;
  std::string post_method_;
  std::string post_data_;

  DISALLOW_COPY_AND_ASSIGN(NetworkState);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_STATE_H_
