// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BLUETOOTH_GATT_DESCRIPTOR_CLIENT_H_
#define CHROMEOS_DBUS_BLUETOOTH_GATT_DESCRIPTOR_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_path.h"
#include "dbus/property.h"

namespace chromeos {

// BluetoothGattDescriptorClient is used to communicate with remote GATT
// characteristic descriptor objects exposed by the Bluetooth daemon.
class CHROMEOS_EXPORT BluetoothGattDescriptorClient : public DBusClient {
 public:
  // Structure of properties associated with GATT descriptors.
  struct Properties : public dbus::PropertySet {
    // The 128-bit characteristic descriptor UUID. [read-only]
    dbus::Property<std::string> uuid;

    // Object path of the GATT characteristic the descriptor belongs to.
    // [read-only]
    dbus::Property<dbus::ObjectPath> characteristic;

    Properties(dbus::ObjectProxy* object_proxy,
               const std::string& interface_name,
               const PropertyChangedCallback& callback);
    virtual ~Properties();
  };

  // Interface for observing changes from a remote GATT characteristic
  // descriptor.
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when the GATT descriptor with object path |object_path| is added
    // to the system.
    virtual void GattDescriptorAdded(const dbus::ObjectPath& object_path) {}

    // Called when the GATT descriptor with object path |object_path| is removed
    // from the system.
    virtual void GattDescriptorRemoved(const dbus::ObjectPath& object_path) {}

    // Called when the GATT descriptor with object path |object_path| has a
    // change in the value of the property named |property_name|.
    virtual void GattDescriptorPropertyChanged(
        const dbus::ObjectPath& object_path,
        const std::string& property_name) {}
  };

  // Callbacks used to report the result of asynchronous methods.
  typedef base::Callback<void(const std::string& error_name,
                              const std::string& error_message)> ErrorCallback;
  typedef base::Callback<void(const std::vector<uint8>& value)> ValueCallback;

  virtual ~BluetoothGattDescriptorClient();

  // Adds and removes observers for events on all remote GATT descriptors. Check
  // the |object_path| parameter of observer methods to determine which GATT
  // descriptor is issuing the event.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Returns the list of GATT descriptor object paths known to the system.
  virtual std::vector<dbus::ObjectPath> GetDescriptors() = 0;

  // Obtain the properties for the GATT descriptor with object path
  // |object_path|. Values should be copied if needed.
  virtual Properties* GetProperties(const dbus::ObjectPath& object_path) = 0;

  // Issues a request to read the value of GATT descriptor with object path
  // |object_path| and returns the value in |callback| on success. On error,
  // invokes |error_callback|.
  virtual void ReadValue(const dbus::ObjectPath& object_path,
                         const ValueCallback& callback,
                         const ErrorCallback& error_callback) = 0;

  // Issues a request to write the value of GATT descriptor with object path
  // |object_path| with value |value|. Invokes |callback| on success and
  // |error_callback| on failure.
  virtual void WriteValue(const dbus::ObjectPath& object_path,
                          const std::vector<uint8>& value,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) = 0;

  // Creates the instance.
  static BluetoothGattDescriptorClient* Create();

  // Constants used to indicate exceptional error conditions.
  static const char kNoResponseError[];
  static const char kUnknownDescriptorError[];

 protected:
  BluetoothGattDescriptorClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothGattDescriptorClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_BLUETOOTH_GATT_DESCRIPTOR_CLIENT_H_
