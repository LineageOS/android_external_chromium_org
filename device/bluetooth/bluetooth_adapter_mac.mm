// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_mac.h"

#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothHostController.h>

#include <string>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/location.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/memory/scoped_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "device/bluetooth/bluetooth_device_mac.h"
#include "device/bluetooth/bluetooth_socket_mac.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace {

const int kPollIntervalMs = 500;

}  // namespace

namespace device {

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapter::CreateAdapter(
    const InitCallback& init_callback) {
  return BluetoothAdapterMac::CreateAdapter();
}

// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapterMac::CreateAdapter() {
  BluetoothAdapterMac* adapter = new BluetoothAdapterMac();
  adapter->Init();
  return adapter->weak_ptr_factory_.GetWeakPtr();
}

BluetoothAdapterMac::BluetoothAdapterMac()
    : BluetoothAdapter(),
      powered_(false),
      num_discovery_sessions_(0),
      classic_discovery_manager_(
          BluetoothDiscoveryManagerMac::CreateClassic(this)),
      weak_ptr_factory_(this) {
  DCHECK(classic_discovery_manager_.get());
}

BluetoothAdapterMac::~BluetoothAdapterMac() {
}

void BluetoothAdapterMac::AddObserver(BluetoothAdapter::Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void BluetoothAdapterMac::RemoveObserver(BluetoothAdapter::Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

std::string BluetoothAdapterMac::GetAddress() const {
  return address_;
}

std::string BluetoothAdapterMac::GetName() const {
  return name_;
}

void BluetoothAdapterMac::SetName(const std::string& name,
                                  const base::Closure& callback,
                                  const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterMac::IsInitialized() const {
  return true;
}

bool BluetoothAdapterMac::IsPresent() const {
  return !address_.empty();
}

bool BluetoothAdapterMac::IsPowered() const {
  return powered_;
}

void BluetoothAdapterMac::SetPowered(bool powered,
                                     const base::Closure& callback,
                                     const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterMac::IsDiscoverable() const {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothAdapterMac::SetDiscoverable(
    bool discoverable,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterMac::IsDiscovering() const {
  return classic_discovery_manager_->IsDiscovering();
}

void BluetoothAdapterMac::CreateRfcommService(
    const BluetoothUUID& uuid,
    int channel,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  scoped_refptr<BluetoothSocketMac> socket = BluetoothSocketMac::CreateSocket();
  socket->ListenUsingRfcomm(
      this, uuid, channel, base::Bind(callback, socket), error_callback);
}

void BluetoothAdapterMac::CreateL2capService(
    const BluetoothUUID& uuid,
    int psm,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  scoped_refptr<BluetoothSocketMac> socket = BluetoothSocketMac::CreateSocket();
  socket->ListenUsingL2cap(
      this, uuid, psm, base::Bind(callback, socket), error_callback);
}

void BluetoothAdapterMac::DeviceFound(BluetoothDiscoveryManagerMac* manager,
                                      IOBluetoothDevice* device) {
  std::string device_address = BluetoothDeviceMac::GetDeviceAddress(device);
  if (discovered_devices_.find(device_address) == discovered_devices_.end()) {
    BluetoothDeviceMac device_mac(device);
    FOR_EACH_OBSERVER(
        BluetoothAdapter::Observer, observers_, DeviceAdded(this, &device_mac));
    discovered_devices_.insert(device_address);
  }
}

void BluetoothAdapterMac::DiscoveryStopped(
    BluetoothDiscoveryManagerMac* manager,
    bool unexpected) {
  DCHECK_EQ(manager, classic_discovery_manager_.get());
  if (unexpected) {
    DVLOG(1) << "Discovery stopped unexpectedly";
    num_discovery_sessions_ = 0;
    MarkDiscoverySessionsAsInactive();
  }
  FOR_EACH_OBSERVER(BluetoothAdapter::Observer,
                    observers_,
                    AdapterDiscoveringChanged(this, false));
}

void BluetoothAdapterMac::AddDiscoverySession(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DVLOG(1) << __func__;
  if (num_discovery_sessions_ > 0) {
    DCHECK(IsDiscovering());
    num_discovery_sessions_++;
    callback.Run();
    return;
  }

  DCHECK_EQ(0, num_discovery_sessions_);

  if (!classic_discovery_manager_->StartDiscovery()) {
    DVLOG(1) << "Failed to add a discovery session";
    error_callback.Run();
    return;
  }

  DVLOG(1) << "Added a discovery session";
  num_discovery_sessions_++;
  FOR_EACH_OBSERVER(BluetoothAdapter::Observer,
                    observers_,
                    AdapterDiscoveringChanged(this, true));
  callback.Run();
}

void BluetoothAdapterMac::RemoveDiscoverySession(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DVLOG(1) << __func__;

  if (num_discovery_sessions_ > 1) {
    // There are active sessions other than the one currently being removed.
    DCHECK(IsDiscovering());
    num_discovery_sessions_--;
    callback.Run();
    return;
  }

  if (num_discovery_sessions_ == 0) {
    DVLOG(1) << "No active discovery sessions. Returning error.";
    error_callback.Run();
    return;
  }

  if (!classic_discovery_manager_->StopDiscovery()) {
    DVLOG(1) << "Failed to stop discovery";
    error_callback.Run();
    return;
  }

  DVLOG(1) << "Discovery stopped";
  num_discovery_sessions_--;
  callback.Run();
}

void BluetoothAdapterMac::RemovePairingDelegateInternal(
    BluetoothDevice::PairingDelegate* pairing_delegate) {
}

void BluetoothAdapterMac::Init() {
  ui_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  PollAdapter();
}

void BluetoothAdapterMac::InitForTest(
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner) {
  ui_task_runner_ = ui_task_runner;
  PollAdapter();
}

void BluetoothAdapterMac::PollAdapter() {
  bool was_present = IsPresent();
  std::string name;
  std::string address;
  bool powered = false;
  IOBluetoothHostController* controller =
      [IOBluetoothHostController defaultController];

  if (controller != nil) {
    name = base::SysNSStringToUTF8([controller nameAsString]);
    address = BluetoothDevice::CanonicalizeAddress(
        base::SysNSStringToUTF8([controller addressAsString]));
    powered = ([controller powerState] == kBluetoothHCIPowerStateON);
  }

  bool is_present = !address.empty();
  name_ = name;
  address_ = address;

  if (was_present != is_present) {
    FOR_EACH_OBSERVER(BluetoothAdapter::Observer, observers_,
                      AdapterPresentChanged(this, is_present));
  }
  if (powered_ != powered) {
    powered_ = powered;
    FOR_EACH_OBSERVER(BluetoothAdapter::Observer, observers_,
                      AdapterPoweredChanged(this, powered_));
  }

  IOBluetoothDevice* recent_device =
      [[IOBluetoothDevice recentDevices:1] lastObject];
  NSDate* access_timestamp = [recent_device recentAccessDate];
  if (recently_accessed_device_timestamp_ == nil ||
      access_timestamp == nil ||
      [recently_accessed_device_timestamp_ compare:access_timestamp] ==
          NSOrderedAscending) {
    UpdateDevices([IOBluetoothDevice pairedDevices]);
    recently_accessed_device_timestamp_.reset([access_timestamp copy]);
  }

  ui_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&BluetoothAdapterMac::PollAdapter,
                 weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kPollIntervalMs));
}

void BluetoothAdapterMac::UpdateDevices(NSArray* devices) {
  // TODO(armansito): This code never calls
  // BluetoothAdapter::Observer::DeviceRemoved. It should, if a device
  // no longer exists.
  STLDeleteValues(&devices_);
  for (IOBluetoothDevice* device in devices) {
    std::string device_address = BluetoothDeviceMac::GetDeviceAddress(device);
    devices_[device_address] = new BluetoothDeviceMac(device);
  }
}

}  // namespace device
