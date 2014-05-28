// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_
#define CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "chromeos/chromeos_export.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace chromeos {

class NetworkUIData;

namespace shill_property_util {

// Sets the |ssid| in |properties|.
CHROMEOS_EXPORT void SetSSID(const std::string ssid,
                             base::DictionaryValue* properties);

// Returns the SSID from |properties| in UTF-8 encoding. If |unknown_encoding|
// is not NULL, it is set to whether the SSID is of unknown encoding.
CHROMEOS_EXPORT std::string GetSSIDFromProperties(
    const base::DictionaryValue& properties,
    bool* unknown_encoding);

// Returns the GUID (if available), SSID, or Name from |properties|. Only used
// for logging and debugging.
CHROMEOS_EXPORT std::string GetNetworkIdFromProperties(
    const base::DictionaryValue& properties);

  // Returns the name for the network represented by the Shill |properties|. For
// WiFi it refers to the HexSSID.
CHROMEOS_EXPORT std::string GetNameFromProperties(
    const std::string& service_path,
    const base::DictionaryValue& properties);

// Returns the UIData specified by |value|. Returns NULL if the value cannot be
// parsed.
scoped_ptr<NetworkUIData> GetUIDataFromValue(const base::Value& value);

// Returns the NetworkUIData parsed from the UIData property of
// |shill_dictionary|. If parsing fails or the field doesn't exist, returns
// NULL.
scoped_ptr<NetworkUIData> GetUIDataFromProperties(
    const base::DictionaryValue& shill_dictionary);

// Sets the UIData property in |shill_dictionary| to the serialization of
// |ui_data|.
void SetUIData(const NetworkUIData& ui_data,
               base::DictionaryValue* shill_dictionary);

// Copy configuration properties required by Shill to identify a network.
// Only WiFi, VPN, Ethernet and EthernetEAP are supported. Wimax and Cellular
// are not supported. Returns true only if all required properties could be
// copied.
bool CopyIdentifyingProperties(const base::DictionaryValue& service_properties,
                               base::DictionaryValue* dest);

// Compares the identifying configuration properties of |properties_a| and
// |properties_b|, returns true if they are identical. See also
// CopyIdentifyingProperties. Only WiFi, VPN, Ethernet and EthernetEAP are
// supported. Wimax and Cellular are not supported.
bool DoIdentifyingPropertiesMatch(const base::DictionaryValue& properties_a,
                                  const base::DictionaryValue& properties_b);

// Returns true if |key| corresponds to a passphrase property.
bool IsPassphraseKey(const std::string& key);

// Parses |value| (which should be a Dictionary). Returns true and sets
// |home_provider_id| if |value| was succesfully parsed.
bool GetHomeProviderFromProperty(const base::Value& value,
                                 std::string* home_provider_id);

}  // namespace shill_property_util

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_
