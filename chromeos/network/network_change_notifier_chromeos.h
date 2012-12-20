// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_CHROMEOS_H_
#define CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_CHROMEOS_H_

#include <string>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "net/base/network_change_notifier.h"

namespace chromeos {

class CHROMEOS_EXPORT NetworkChangeNotifierChromeos
    : public net::NetworkChangeNotifier,
      public chromeos::NetworkStateHandlerObserver {
 public:
  NetworkChangeNotifierChromeos();
  virtual ~NetworkChangeNotifierChromeos();

  // Starts observing changes from the network state handler.
  void Initialize();

  // Stops observing changes from the network state handler.
  void Shutdown();

  // NetworkChangeNotifier overrides.
  net::NetworkChangeNotifier::ConnectionType
      GetCurrentConnectionType() const OVERRIDE;

  // NetworkStateHandlerObserver overrides.
  void ActiveNetworkChanged(
      const chromeos::NetworkState* active_network) OVERRIDE;
  void ActiveNetworkStateChanged(
      const chromeos::NetworkState* active_network) OVERRIDE;

 private:
  FRIEND_TEST_ALL_PREFIXES(NetworkChangeNotifierChromeosTest,
                           ConnectionTypeFromShill);
  friend class NetworkChangeNotifierChromeosUpdateTest;

  class DnsConfigService;

  // Updates the notifier state based on an active network update.
  // |connection_type_changed| is set to true if we must report a connection
  // type change.
  // |ip_address_changed| is set to true if we must report an IP address change.
  // |dns_changed| is set to true if we must report a DNS config change.
  void UpdateState(const chromeos::NetworkState* active_network,
                   bool* connection_type_changed,
                   bool* ip_address_changed,
                   bool* dns_changed);

  // Maps the shill network type and technology to its NetworkChangeNotifier
  // equivalent.
  static net::NetworkChangeNotifier::ConnectionType
      ConnectionTypeFromShill(const std::string& type,
                              const std::string& technology);

  // Calculates parameters used for network change notifier online/offline
  // signals.
  static net::NetworkChangeNotifier::NetworkChangeCalculatorParams
      NetworkChangeCalculatorParamsChromeos();

  NetworkChangeNotifier::ConnectionType connection_type_;
  // IP address for the current active network.
  std::string ip_address_;
  // Service path for the current active network.
  std::string service_path_;

  scoped_ptr<DnsConfigService> dns_config_service_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierChromeos);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_CHANGE_NOTIFIER_CHROMEOS_H_
