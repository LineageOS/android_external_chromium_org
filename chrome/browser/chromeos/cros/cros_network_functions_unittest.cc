// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/chromeos/cros/cros_network_functions.h"
#include "chrome/browser/chromeos/cros/gvalue_util.h"
#include "chrome/browser/chromeos/cros/mock_chromeos_network.h"
#include "chromeos/dbus/mock_cashew_client.h"
#include "chromeos/dbus/mock_dbus_thread_manager.h"
#include "chromeos/dbus/mock_flimflam_device_client.h"
#include "chromeos/dbus/mock_flimflam_ipconfig_client.h"
#include "chromeos/dbus/mock_flimflam_manager_client.h"
#include "chromeos/dbus/mock_flimflam_profile_client.h"
#include "chromeos/dbus/mock_flimflam_service_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrEq;

// Matcher to match base::Value.
MATCHER_P(IsEqualTo, value, "") { return arg.Equals(value); }

namespace chromeos {

namespace {

const char kExamplePath[] = "/foo/bar/baz";

// A mock to check arguments of NetworkPropertiesCallback and ensure that the
// callback is called exactly once.
class MockNetworkPropertiesCallback {
 public:
  // Creates a NetworkPropertiesCallback with expectations.
  static NetworkPropertiesCallback CreateCallback(
      const std::string& expected_path,
      const base::DictionaryValue& expected_result) {
    MockNetworkPropertiesCallback* mock_callback =
        new MockNetworkPropertiesCallback;

    EXPECT_CALL(*mock_callback,
                Run(expected_path, Pointee(IsEqualTo(&expected_result))))
        .Times(1);

    return base::Bind(&MockNetworkPropertiesCallback::Run,
                      base::Owned(mock_callback));
  }

  MOCK_METHOD2(Run, void(const std::string& path,
                         const base::DictionaryValue* result));
};

// A mock to check arguments of NetworkPropertiesWatcherCallback and ensure that
// the callback is called exactly once.
class MockNetworkPropertiesWatcherCallback {
 public:
  // Creates a NetworkPropertiesWatcherCallback with expectations.
  static NetworkPropertiesWatcherCallback CreateCallback(
      const std::string& expected_path,
      const std::string& expected_key,
      const base::Value& expected_value) {
    MockNetworkPropertiesWatcherCallback* mock_callback =
        new MockNetworkPropertiesWatcherCallback;

    EXPECT_CALL(*mock_callback,
                Run(expected_path, expected_key, IsEqualTo(&expected_value)))
        .Times(1);

    return base::Bind(&MockNetworkPropertiesWatcherCallback::Run,
                      base::Owned(mock_callback));
  }

  MOCK_METHOD3(Run, void(const std::string& expected_path,
                         const std::string& expected_key,
                         const base::Value& value));
};

}  // namespace

// Test for cros_network_functions.cc with Libcros.
class CrosNetworkFunctionsLibcrosTest : public testing::Test {
 public:
  CrosNetworkFunctionsLibcrosTest() : argument_ghash_table_(NULL) {}

  virtual void SetUp() {
    ::g_type_init();
    MockChromeOSNetwork::Initialize();
    SetLibcrosNetworkFunctionsEnabled(true);
  }

  virtual void TearDown() {
    MockChromeOSNetwork::Shutdown();
  }

  // Handles call for SetNetwork*PropertyGValue functions and copies the value.
  void OnSetNetworkPropertyGValue(const char* path,
                                  const char* property,
                                  const GValue* gvalue) {
    argument_gvalue_.reset(new GValue());
    g_value_init(argument_gvalue_.get(), G_VALUE_TYPE(gvalue));
    g_value_copy(gvalue, argument_gvalue_.get());
  }

  // Handles call for SetNetworkManagerPropertyGValue.
  void OnSetNetworkManagerPropertyGValue(const char* property,
                                         const GValue* gvalue) {
    OnSetNetworkPropertyGValue("", property, gvalue);
  }

  // Handles call for MonitorNetworkManagerProperties
  NetworkPropertiesMonitor OnMonitorNetworkManagerProperties(
      MonitorPropertyGValueCallback callback, void* object) {
    property_changed_callback_ = base::Bind(callback, object);
    return NULL;
  }

  // Handles call for MonitorNetwork*Properties
  NetworkPropertiesMonitor OnMonitorNetworkProperties(
      MonitorPropertyGValueCallback callback, const char* path, void* object) {
    property_changed_callback_ = base::Bind(callback, object);
    return NULL;
  }

  // Handles call for ConfigureService.
  void OnConfigureService(const char* identifier,
                          const GHashTable* properties,
                          NetworkActionCallback callback,
                          void* object) {
    // const_cast here because nothing can be done with const GHashTable*.
    argument_ghash_table_.reset(const_cast<GHashTable*>(properties));
    g_hash_table_ref(argument_ghash_table_.get());
  }

  // Handles call for RequestNetwork*Properties.
  void OnRequestNetworkProperties2(NetworkPropertiesGValueCallback callback,
                                   void* object) {
    callback(object, kExamplePath, result_ghash_table_.get());
  }

  // Handles call for RequestNetwork*Properties, ignores the first argument.
  void OnRequestNetworkProperties3(const char* arg1,
                                   NetworkPropertiesGValueCallback callback,
                                   void* object) {
    OnRequestNetworkProperties2(callback, object);
  }

  // Handles call for RequestNetwork*Properties, ignores the first two
  // arguments.
  void OnRequestNetworkProperties4(const char* arg1,
                                   const char* arg2,
                                   NetworkPropertiesGValueCallback callback,
                                   void* object) {
    OnRequestNetworkProperties2(callback, object);
  }

  // Handles call for RequestNetwork*Properties, ignores the first three
  // arguments.
  void OnRequestNetworkProperties5(const char* arg1,
                                   const char* arg2,
                                   const char* arg3,
                                   NetworkPropertiesGValueCallback callback,
                                   void* object) {
    OnRequestNetworkProperties2(callback, object);
  }

  // Does nothing.  Used as an argument.
  static void OnNetworkAction(void* object,
                              const char* path,
                              NetworkMethodErrorType error,
                              const char* error_message) {}

 protected:
  ScopedGValue argument_gvalue_;
  ScopedGHashTable argument_ghash_table_;
  ScopedGHashTable result_ghash_table_;
  base::Callback<void(const char* path,
                      const char* key,
                      const GValue* gvalue)> property_changed_callback_;
};

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosSetNetworkServiceProperty) {
  const std::string service_path = "/";
  const std::string property = "property";
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      SetNetworkServicePropertyGValue(StrEq(service_path), StrEq(property), _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnSetNetworkPropertyGValue));
  const std::string key1= "key1";
  const std::string string1 = "string1";
  const std::string key2 = "key2";
  const std::string string2 = "string2";
  base::DictionaryValue value;
  value.SetString(key1, string1);
  value.SetString(key2, string2);
  CrosSetNetworkServiceProperty(service_path, property, value);

  // Check the argument GHashTable.
  GHashTable* ghash_table =
      static_cast<GHashTable*>(g_value_get_boxed(argument_gvalue_.get()));
  ASSERT_TRUE(ghash_table != NULL);
  EXPECT_EQ(string1, static_cast<const char*>(
      g_hash_table_lookup(ghash_table, key1.c_str())));
  EXPECT_EQ(string2, static_cast<const char*>(
      g_hash_table_lookup(ghash_table, key2.c_str())));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosClearNetworkServiceProperty) {
  const std::string service_path = "/";
  const std::string property = "property";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              ClearNetworkServiceProperty(StrEq(service_path),
                                          StrEq(property))).Times(1);
  CrosClearNetworkServiceProperty(service_path, property);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosSetNetworkDeviceProperty) {
  const std::string device_path = "/";
  const std::string property = "property";
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      SetNetworkDevicePropertyGValue(StrEq(device_path), StrEq(property), _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnSetNetworkPropertyGValue));
  const bool kBool = true;
  const base::FundamentalValue value(kBool);
  CrosSetNetworkDeviceProperty(device_path, property, value);
  EXPECT_EQ(kBool, g_value_get_boolean(argument_gvalue_.get()));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosSetNetworkIPConfigProperty) {
  const std::string ipconfig_path = "/";
  const std::string property = "property";
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      SetNetworkIPConfigPropertyGValue(StrEq(ipconfig_path),
                                       StrEq(property), _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnSetNetworkPropertyGValue));
  const int kInt = 1234;
  const base::FundamentalValue value(kInt);
  CrosSetNetworkIPConfigProperty(ipconfig_path, property, value);
  EXPECT_EQ(kInt, g_value_get_int(argument_gvalue_.get()));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosSetNetworkManagerProperty) {
  const std::string property = "property";
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      SetNetworkManagerPropertyGValue(StrEq(property), _))
      .WillOnce(Invoke(
          this,
          &CrosNetworkFunctionsLibcrosTest::OnSetNetworkManagerPropertyGValue));
  const std::string str = "string";
  const base::StringValue value(str);
  CrosSetNetworkManagerProperty(property, value);
  EXPECT_EQ(str, g_value_get_string(argument_gvalue_.get()));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestCellularDataPlanUpdate) {
  const std::string modem_service_path = "/modem/service/path";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestCellularDataPlanUpdate(StrEq(modem_service_path)))
      .Times(1);
  CrosRequestCellularDataPlanUpdate(modem_service_path);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosMonitorNetworkManagerProperties) {
  const std::string path = "path";
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);

  // Start monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              MonitorNetworkManagerProperties(_, _))
      .WillOnce(Invoke(
          this,
          &CrosNetworkFunctionsLibcrosTest::OnMonitorNetworkManagerProperties));
  CrosNetworkWatcher* watcher = CrosMonitorNetworkManagerProperties(
      MockNetworkPropertiesWatcherCallback::CreateCallback(path, key, value));

  // Call callback.
  ScopedGValue gvalue(new GValue());
  g_value_init(gvalue.get(), G_TYPE_INT);
  g_value_set_int(gvalue.get(), kValue);
  property_changed_callback_.Run(path.c_str(), key.c_str(), gvalue.get());

  // Stop monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              DisconnectNetworkPropertiesMonitor(_)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosMonitorNetworkServiceProperties) {
  const std::string path = "path";
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);

  // Start monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              MonitorNetworkServiceProperties(_, StrEq(path), _))
      .WillOnce(Invoke(
          this,
          &CrosNetworkFunctionsLibcrosTest::OnMonitorNetworkProperties));
  CrosNetworkWatcher* watcher = CrosMonitorNetworkServiceProperties(
      MockNetworkPropertiesWatcherCallback::CreateCallback(path, key, value),
      path);

  // Call callback.
  ScopedGValue gvalue(new GValue());
  g_value_init(gvalue.get(), G_TYPE_INT);
  g_value_set_int(gvalue.get(), kValue);
  property_changed_callback_.Run(path.c_str(), key.c_str(), gvalue.get());

  // Stop monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              DisconnectNetworkPropertiesMonitor(_)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosMonitorNetworkDeviceProperties) {
  const std::string path = "path";
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);

  // Start monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              MonitorNetworkDeviceProperties(_, StrEq(path), _))
      .WillOnce(Invoke(
          this,
          &CrosNetworkFunctionsLibcrosTest::OnMonitorNetworkProperties));
  CrosNetworkWatcher* watcher = CrosMonitorNetworkDeviceProperties(
      MockNetworkPropertiesWatcherCallback::CreateCallback(path, key, value),
      path);

  // Call callback.
  ScopedGValue gvalue(new GValue());
  g_value_init(gvalue.get(), G_TYPE_INT);
  g_value_set_int(gvalue.get(), kValue);
  property_changed_callback_.Run(path.c_str(), key.c_str(), gvalue.get());

  // Stop monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              DisconnectNetworkPropertiesMonitor(_)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosMonitorCellularDataPlan) {
  MonitorDataPlanCallback callback =
      reinterpret_cast<MonitorDataPlanCallback>(42);  // Dummy value.
  void* object = reinterpret_cast<void*>(84);  // Dummy value.

  // Start monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              MonitorCellularDataPlan(callback, object)).Times(1);
  CrosNetworkWatcher* watcher = CrosMonitorCellularDataPlan(callback, object);

  // Stop monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              DisconnectDataPlanUpdateMonitor(_)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosMonitorSMS) {
  const std::string modem_device_path = "/modem/device/path";
  MonitorSMSCallback callback =
      reinterpret_cast<MonitorSMSCallback>(42);  // Dummy value.
  void* object = reinterpret_cast<void*>(84);  // Dummy value.

  // Start monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              MonitorSMS(StrEq(modem_device_path), callback, object))
      .Times(1);
  CrosNetworkWatcher* watcher = CrosMonitorSMS(
      modem_device_path, callback, object);

  // Stop monitoring.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              DisconnectSMSMonitor(_)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkManagerProperties) {
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestNetworkManagerProperties(_, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties2));

  CrosRequestNetworkManagerProperties(
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkServiceProperties) {
  const std::string service_path = "service path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestNetworkServiceProperties(StrEq(service_path), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties3));

  CrosRequestNetworkServiceProperties(
      service_path,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkDeviceProperties) {
  const std::string device_path = "device path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestNetworkDeviceProperties(StrEq(device_path), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties3));

  CrosRequestNetworkDeviceProperties(
      device_path,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkProfileProperties) {
  const std::string profile_path = "profile path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestNetworkProfileProperties(StrEq(profile_path), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties3));

  CrosRequestNetworkProfileProperties(
      profile_path,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest,
       CrosRequestNetworkProfileEntryProperties) {
  const std::string profile_path = "profile path";
  const std::string profile_entry_path = "profile entry path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestNetworkProfileEntryProperties(
          StrEq(profile_path), StrEq(profile_entry_path), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties4));

  CrosRequestNetworkProfileEntryProperties(
      profile_path,
      profile_entry_path,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest,
       CrosRequestHiddenWifiNetworkProperties) {
  const std::string ssid = "ssid";
  const std::string security = "security";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(
      *MockChromeOSNetwork::Get(),
      RequestHiddenWifiNetworkProperties(
          StrEq(ssid), StrEq(security), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties4));

  CrosRequestHiddenWifiNetworkProperties(
      ssid, security,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestVirtualNetworkProperties) {
  const std::string service_name = "service name";
  const std::string server_hostname = "server hostname";
  const std::string provider_name = "provider name";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  result_ghash_table_.reset(
      ConvertDictionaryValueToStringValueGHashTable(result));
  // Set expectations.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestVirtualNetworkProperties(
                  StrEq(service_name), StrEq(server_hostname),
                  StrEq(provider_name), _, _))
      .WillOnce(Invoke(
          this, &CrosNetworkFunctionsLibcrosTest::OnRequestNetworkProperties5));

  CrosRequestVirtualNetworkProperties(
      service_name,
      server_hostname,
      provider_name,
      MockNetworkPropertiesCallback::CreateCallback(kExamplePath, result));
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkServiceDisconnect) {
  const std::string service_path = "/service/path";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestNetworkServiceDisconnect(StrEq(service_path))).Times(1);
  CrosRequestNetworkServiceDisconnect(service_path);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestRemoveNetworkService) {
  const std::string service_path = "/service/path";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestRemoveNetworkService(StrEq(service_path))).Times(1);
  CrosRequestRemoveNetworkService(service_path);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkScan) {
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestNetworkScan(StrEq(flimflam::kTypeWifi))).Times(1);
  CrosRequestNetworkScan(flimflam::kTypeWifi);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRequestNetworkDeviceEnable) {
  const bool kEnable = true;
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RequestNetworkDeviceEnable(StrEq(flimflam::kTypeWifi), kEnable))
      .Times(1);
  CrosRequestNetworkDeviceEnable(flimflam::kTypeWifi, kEnable);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosAddIPConfig) {
  const std::string device_path = "/device/path";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              AddIPConfig(StrEq(device_path), IPCONFIG_TYPE_DHCP)).Times(1);
  CrosAddIPConfig(device_path, IPCONFIG_TYPE_DHCP);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosRemoveIPConfig) {
  IPConfig config = {};
  config.path = "/path";
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              RemoveIPConfig(&config)).Times(1);
  CrosRemoveIPConfig(&config);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosGetWifiAccessPoints) {
  // Preapare data.
  std::vector<DeviceNetworkInfo> networks(1);
  networks[0].device_path = "/device/path";
  networks[0].network_path = "/network/path";
  networks[0].address = "address";
  networks[0].name = "name";
  networks[0].strength = 42;
  networks[0].channel = 24;
  networks[0].connected = true;
  networks[0].age_seconds = 12345;

  DeviceNetworkList network_list;
  network_list.network_size = networks.size();
  network_list.networks = &networks[0];

  const base::Time expected_timestamp =
      base::Time::Now() - base::TimeDelta::FromSeconds(networks[0].age_seconds);
  const base::TimeDelta acceptable_timestamp_range =
      base::TimeDelta::FromSeconds(1);

  // Set expectations.
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              GetDeviceNetworkList()).WillOnce(Return(&network_list));
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              FreeDeviceNetworkList(&network_list)).Times(1);

  // Call function.
  WifiAccessPointVector aps;
  ASSERT_TRUE(CrosGetWifiAccessPoints(&aps));
  ASSERT_EQ(1U, aps.size());
  EXPECT_EQ(networks[0].address, aps[0].mac_address);
  EXPECT_EQ(networks[0].name, aps[0].name);
  EXPECT_LE(expected_timestamp - acceptable_timestamp_range, aps[0].timestamp);
  EXPECT_GE(expected_timestamp + acceptable_timestamp_range, aps[0].timestamp);
  EXPECT_EQ(networks[0].strength, aps[0].signal_strength);
  EXPECT_EQ(networks[0].channel, aps[0].channel);
}

TEST_F(CrosNetworkFunctionsLibcrosTest, CrosConfigureService) {
  EXPECT_CALL(*MockChromeOSNetwork::Get(),
              ConfigureService(_, _, _, _))
              .WillOnce(Invoke(
                  this, &CrosNetworkFunctionsLibcrosTest::OnConfigureService));
  const std::string key1 = "key1";
  const std::string string1 = "string1";
  const std::string key2 = "key2";
  const std::string string2 = "string2";
  base::DictionaryValue value;
  value.SetString(key1, string1);
  value.SetString(key2, string2);
  CrosConfigureService(value);

  // Check the argument GHashTable.
  const GValue* string1_gvalue = static_cast<const GValue*>(
      g_hash_table_lookup(argument_ghash_table_.get(), key1.c_str()));
  EXPECT_EQ(string1, g_value_get_string(string1_gvalue));
  const GValue* string2_gvalue = static_cast<const GValue*>(
      g_hash_table_lookup(argument_ghash_table_.get(), key2.c_str()));
  EXPECT_EQ(string2, g_value_get_string(string2_gvalue));
}

// Test for cros_network_functions.cc without Libcros.
class CrosNetworkFunctionsTest : public testing::Test {
 public:
  CrosNetworkFunctionsTest() : mock_profile_client_(NULL),
                               dictionary_value_result_(NULL) {}

  virtual void SetUp() {
    MockDBusThreadManager* mock_dbus_thread_manager = new MockDBusThreadManager;
    DBusThreadManager::InitializeForTesting(mock_dbus_thread_manager);
    mock_cashew_client_ = mock_dbus_thread_manager->mock_cashew_client();
    mock_device_client_ =
        mock_dbus_thread_manager->mock_flimflam_device_client();
    mock_ipconfig_client_ =
        mock_dbus_thread_manager->mock_flimflam_ipconfig_client();
    mock_manager_client_ =
        mock_dbus_thread_manager->mock_flimflam_manager_client();
    mock_profile_client_ =
        mock_dbus_thread_manager->mock_flimflam_profile_client();
    mock_service_client_ =
        mock_dbus_thread_manager->mock_flimflam_service_client();
    SetLibcrosNetworkFunctionsEnabled(false);
  }

  virtual void TearDown() {
    DBusThreadManager::Shutdown();
    mock_profile_client_ = NULL;
  }

  // Handles responses for GetProperties method calls for FlimflamManagerClient.
  void OnGetManagerProperties(
      const FlimflamClientHelper::DictionaryValueCallback& callback) {
    callback.Run(DBUS_METHOD_CALL_SUCCESS, *dictionary_value_result_);
  }

  // Handles responses for GetProperties method calls.
  void OnGetProperties(
      const dbus::ObjectPath& path,
      const FlimflamClientHelper::DictionaryValueCallback& callback) {
    callback.Run(DBUS_METHOD_CALL_SUCCESS, *dictionary_value_result_);
  }

  // Handles responses for GetEntry method calls.
  void OnGetEntry(
      const dbus::ObjectPath& profile_path,
      const std::string& entry_path,
      const FlimflamClientHelper::DictionaryValueCallback& callback) {
    callback.Run(DBUS_METHOD_CALL_SUCCESS, *dictionary_value_result_);
  }

 protected:
  MockCashewClient* mock_cashew_client_;
  MockFlimflamDeviceClient* mock_device_client_;
  MockFlimflamIPConfigClient* mock_ipconfig_client_;
  MockFlimflamManagerClient* mock_manager_client_;
  MockFlimflamProfileClient* mock_profile_client_;
  MockFlimflamServiceClient* mock_service_client_;
  const base::DictionaryValue* dictionary_value_result_;
};

TEST_F(CrosNetworkFunctionsTest, CrosSetNetworkServiceProperty) {
  const std::string service_path = "/";
  const std::string property = "property";
  const std::string key1 = "key1";
  const std::string string1 = "string1";
  const std::string key2 = "key2";
  const std::string string2 = "string2";
  base::DictionaryValue value;
  value.SetString(key1, string1);
  value.SetString(key2, string2);
  EXPECT_CALL(*mock_service_client_,
              SetProperty(dbus::ObjectPath(service_path), property,
                          IsEqualTo(&value), _)).Times(1);

  CrosSetNetworkServiceProperty(service_path, property, value);
}

TEST_F(CrosNetworkFunctionsTest, CrosClearNetworkServiceProperty) {
  const std::string service_path = "/";
  const std::string property = "property";
  EXPECT_CALL(*mock_service_client_,
              ClearProperty(dbus::ObjectPath(service_path), property, _))
      .Times(1);

  CrosClearNetworkServiceProperty(service_path, property);
}

TEST_F(CrosNetworkFunctionsTest, CrosSetNetworkDeviceProperty) {
  const std::string device_path = "/";
  const std::string property = "property";
  const bool kBool = true;
  const base::FundamentalValue value(kBool);
  EXPECT_CALL(*mock_device_client_,
              SetProperty(dbus::ObjectPath(device_path), StrEq(property),
                          IsEqualTo(&value), _)).Times(1);

  CrosSetNetworkDeviceProperty(device_path, property, value);
}

TEST_F(CrosNetworkFunctionsTest, CrosSetNetworkIPConfigProperty) {
  const std::string ipconfig_path = "/";
  const std::string property = "property";
  const int kInt = 1234;
  const base::FundamentalValue value(kInt);
  EXPECT_CALL(*mock_ipconfig_client_,
              SetProperty(dbus::ObjectPath(ipconfig_path), property,
                          IsEqualTo(&value), _)).Times(1);
  CrosSetNetworkIPConfigProperty(ipconfig_path, property, value);
}

TEST_F(CrosNetworkFunctionsTest, CrosSetNetworkManagerProperty) {
  const std::string property = "property";
  const base::StringValue value("string");
  EXPECT_CALL(*mock_manager_client_,
              SetProperty(property, IsEqualTo(&value), _)).Times(1);

  CrosSetNetworkManagerProperty(property, value);
}

TEST_F(CrosNetworkFunctionsTest, CrosDeleteServiceFromProfile) {
  const std::string profile_path("/profile/path");
  const std::string service_path("/service/path");
  EXPECT_CALL(*mock_profile_client_,
              DeleteEntry(dbus::ObjectPath(profile_path), service_path, _))
      .Times(1);
  CrosDeleteServiceFromProfile(profile_path, service_path);
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestCellularDataPlanUpdate) {
  const std::string modem_service_path = "/modem/service/path";
  EXPECT_CALL(*mock_cashew_client_,
              RequestDataPlansUpdate(modem_service_path)).Times(1);
  CrosRequestCellularDataPlanUpdate(modem_service_path);
}

TEST_F(CrosNetworkFunctionsTest, CrosMonitorNetworkManagerProperties) {
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);
  // Start monitoring.
  FlimflamClientHelper::PropertyChangedHandler handler;
  EXPECT_CALL(*mock_manager_client_, SetPropertyChangedHandler(_))
      .WillOnce(SaveArg<0>(&handler));
  CrosNetworkWatcher* watcher = CrosMonitorNetworkManagerProperties(
      MockNetworkPropertiesWatcherCallback::CreateCallback(
          flimflam::kFlimflamServicePath, key, value));
  // Call callback.
  handler.Run(key, value);
  // Stop monitoring.
  EXPECT_CALL(*mock_manager_client_, ResetPropertyChangedHandler()).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsTest, CrosMonitorNetworkServiceProperties) {
  const dbus::ObjectPath path("/path");
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);
  // Start monitoring.
  FlimflamClientHelper::PropertyChangedHandler handler;
  EXPECT_CALL(*mock_service_client_, SetPropertyChangedHandler(path, _))
      .WillOnce(SaveArg<1>(&handler));
  NetworkPropertiesWatcherCallback callback =
      MockNetworkPropertiesWatcherCallback::CreateCallback(path.value(),
                                                           key, value);
  CrosNetworkWatcher* watcher = CrosMonitorNetworkServiceProperties(
      callback, path.value());
  // Call callback.
  handler.Run(key, value);
  // Stop monitoring.
  EXPECT_CALL(*mock_service_client_,
              ResetPropertyChangedHandler(path)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsTest, CrosMonitorNetworkDeviceProperties) {
  const dbus::ObjectPath path("/path");
  const std::string key = "key";
  const int kValue = 42;
  const base::FundamentalValue value(kValue);
  // Start monitoring.
  FlimflamClientHelper::PropertyChangedHandler handler;
  EXPECT_CALL(*mock_device_client_, SetPropertyChangedHandler(path, _))
      .WillOnce(SaveArg<1>(&handler));
  NetworkPropertiesWatcherCallback callback =
      MockNetworkPropertiesWatcherCallback::CreateCallback(path.value(),
                                                           key, value);
  CrosNetworkWatcher* watcher = CrosMonitorNetworkDeviceProperties(
      callback, path.value());
  // Call callback.
  handler.Run(key, value);
  // Stop monitoring.
  EXPECT_CALL(*mock_device_client_,
              ResetPropertyChangedHandler(path)).Times(1);
  delete watcher;
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkManagerProperties) {
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  // Set expectations.
  dictionary_value_result_ = &result;
  EXPECT_CALL(*mock_manager_client_,
              GetProperties(_)).WillOnce(
                  Invoke(this,
                         &CrosNetworkFunctionsTest::OnGetManagerProperties));

  CrosRequestNetworkManagerProperties(
      MockNetworkPropertiesCallback::CreateCallback(
          flimflam::kFlimflamServicePath, result));
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkServiceProperties) {
  const std::string service_path = "/service/path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  // Set expectations.
  dictionary_value_result_ = &result;
  EXPECT_CALL(*mock_service_client_,
              GetProperties(dbus::ObjectPath(service_path), _)).WillOnce(
                  Invoke(this, &CrosNetworkFunctionsTest::OnGetProperties));

  CrosRequestNetworkServiceProperties(
      service_path,
      MockNetworkPropertiesCallback::CreateCallback(service_path, result));
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkDeviceProperties) {
  const std::string device_path = "/device/path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  // Set expectations.
  dictionary_value_result_ = &result;
  EXPECT_CALL(*mock_device_client_,
              GetProperties(dbus::ObjectPath(device_path), _)).WillOnce(
                  Invoke(this, &CrosNetworkFunctionsTest::OnGetProperties));

  CrosRequestNetworkDeviceProperties(
      device_path,
      MockNetworkPropertiesCallback::CreateCallback(device_path, result));
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkProfileProperties) {
  const std::string profile_path = "/profile/path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  // Set expectations.
  dictionary_value_result_ = &result;
  EXPECT_CALL(*mock_profile_client_,
              GetProperties(dbus::ObjectPath(profile_path), _)).WillOnce(
                  Invoke(this, &CrosNetworkFunctionsTest::OnGetProperties));

  CrosRequestNetworkProfileProperties(
      profile_path,
      MockNetworkPropertiesCallback::CreateCallback(profile_path, result));
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkProfileEntryProperties) {
  const std::string profile_path = "profile path";
  const std::string profile_entry_path = "profile entry path";
  const std::string key1 = "key1";
  const std::string value1 = "value1";
  const std::string key2 = "key.2.";
  const std::string value2 = "value2";
  // Create result value.
  base::DictionaryValue result;
  result.SetWithoutPathExpansion(key1, base::Value::CreateStringValue(value1));
  result.SetWithoutPathExpansion(key2, base::Value::CreateStringValue(value2));
  // Set expectations.
  dictionary_value_result_ = &result;
  EXPECT_CALL(*mock_profile_client_,
              GetEntry(dbus::ObjectPath(profile_path), profile_entry_path, _))
      .WillOnce(Invoke(this, &CrosNetworkFunctionsTest::OnGetEntry));

  CrosRequestNetworkProfileEntryProperties(
      profile_path, profile_entry_path,
      MockNetworkPropertiesCallback::CreateCallback(profile_entry_path,
                                                    result));
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkServiceDisconnect) {
  const std::string service_path = "/service/path";
  EXPECT_CALL(*mock_service_client_,
              Disconnect(dbus::ObjectPath(service_path), _)).Times(1);
  CrosRequestNetworkServiceDisconnect(service_path);
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestRemoveNetworkService) {
  const std::string service_path = "/service/path";
  EXPECT_CALL(*mock_service_client_,
              Remove(dbus::ObjectPath(service_path), _)).Times(1);
  CrosRequestRemoveNetworkService(service_path);
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkScan) {
  EXPECT_CALL(*mock_manager_client_,
              RequestScan(flimflam::kTypeWifi, _)).Times(1);
  CrosRequestNetworkScan(flimflam::kTypeWifi);
}

TEST_F(CrosNetworkFunctionsTest, CrosRequestNetworkDeviceEnable) {
  const bool kEnable = true;
  EXPECT_CALL(*mock_manager_client_,
              EnableTechnology(flimflam::kTypeWifi, _)).Times(1);
  CrosRequestNetworkDeviceEnable(flimflam::kTypeWifi, kEnable);

  const bool kDisable = false;
  EXPECT_CALL(*mock_manager_client_,
              DisableTechnology(flimflam::kTypeWifi, _)).Times(1);
  CrosRequestNetworkDeviceEnable(flimflam::kTypeWifi, kDisable);
}

TEST_F(CrosNetworkFunctionsTest, CrosAddIPConfig) {
  const std::string device_path = "/device/path";
  const dbus::ObjectPath result_path("/result/path");
  EXPECT_CALL(*mock_device_client_,
              CallAddIPConfigAndBlock(dbus::ObjectPath(device_path),
                                      flimflam::kTypeDHCP))
      .WillOnce(Return(result_path));
  EXPECT_TRUE(CrosAddIPConfig(device_path, IPCONFIG_TYPE_DHCP));
}

TEST_F(CrosNetworkFunctionsTest, CrosRemoveIPConfig) {
  IPConfig config = {};
  config.path = "/path";
  EXPECT_CALL(*mock_ipconfig_client_,
              CallRemoveAndBlock(dbus::ObjectPath(config.path))).Times(1);
  CrosRemoveIPConfig(&config);
}

TEST_F(CrosNetworkFunctionsTest, CrosConfigureService) {
  const std::string key1 = "key1";
  const std::string string1 = "string1";
  const std::string key2 = "key2";
  const std::string string2 = "string2";
  base::DictionaryValue value;
  value.SetString(key1, string1);
  value.SetString(key2, string2);
  EXPECT_CALL(*mock_manager_client_, ConfigureService(IsEqualTo(&value), _))
      .Times(1);
  CrosConfigureService(value);
}

}  // namespace chromeos
