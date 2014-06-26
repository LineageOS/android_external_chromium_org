// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/bluetooth/bluetooth_api.h"

#include <string>

#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/api/bluetooth/bluetooth_api_utils.h"
#include "chrome/browser/extensions/api/bluetooth/bluetooth_event_router.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/extensions/api/bluetooth.h"
#include "content/public/browser/browser_thread.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "extensions/browser/event_router.h"

using content::BrowserContext;
using content::BrowserThread;

using device::BluetoothAdapter;
using device::BluetoothDevice;

namespace bluetooth = extensions::api::bluetooth;
namespace GetDevice = extensions::api::bluetooth::GetDevice;
namespace GetDevices = extensions::api::bluetooth::GetDevices;

namespace {

const char kInvalidDevice[] = "Invalid device";
const char kStartDiscoveryFailed[] = "Starting discovery failed";
const char kStopDiscoveryFailed[] = "Failed to stop discovery";

extensions::BluetoothEventRouter* GetEventRouter(BrowserContext* context) {
  // Note: |context| is valid on UI thread only.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return extensions::BluetoothAPI::Get(context)->event_router();
}

}  // namespace

namespace extensions {

static base::LazyInstance<BrowserContextKeyedAPIFactory<BluetoothAPI> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<BluetoothAPI>*
BluetoothAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
BluetoothAPI* BluetoothAPI::Get(BrowserContext* context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return GetFactoryInstance()->Get(context);
}

BluetoothAPI::BluetoothAPI(content::BrowserContext* context)
    : browser_context_(context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  EventRouter* event_router = EventRouter::Get(browser_context_);
  event_router->RegisterObserver(this,
                                 bluetooth::OnAdapterStateChanged::kEventName);
  event_router->RegisterObserver(this, bluetooth::OnDeviceAdded::kEventName);
  event_router->RegisterObserver(this, bluetooth::OnDeviceChanged::kEventName);
  event_router->RegisterObserver(this, bluetooth::OnDeviceRemoved::kEventName);
}

BluetoothAPI::~BluetoothAPI() {}

BluetoothEventRouter* BluetoothAPI::event_router() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!event_router_) {
    event_router_.reset(new BluetoothEventRouter(browser_context_));
  }
  return event_router_.get();
}

void BluetoothAPI::Shutdown() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

void BluetoothAPI::OnListenerAdded(const EventListenerInfo& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (event_router()->IsBluetoothSupported())
    event_router()->OnListenerAdded();
}

void BluetoothAPI::OnListenerRemoved(const EventListenerInfo& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (event_router()->IsBluetoothSupported())
    event_router()->OnListenerRemoved();
}

namespace api {

BluetoothGetAdapterStateFunction::~BluetoothGetAdapterStateFunction() {}

bool BluetoothGetAdapterStateFunction::DoWork(
    scoped_refptr<BluetoothAdapter> adapter) {
  bluetooth::AdapterState state;
  PopulateAdapterState(*adapter.get(), &state);
  results_ = bluetooth::GetAdapterState::Results::Create(state);
  SendResponse(true);
  return true;
}

BluetoothGetDevicesFunction::~BluetoothGetDevicesFunction() {}

bool BluetoothGetDevicesFunction::DoWork(
    scoped_refptr<BluetoothAdapter> adapter) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  base::ListValue* device_list = new base::ListValue;
  SetResult(device_list);

  BluetoothAdapter::DeviceList devices = adapter->GetDevices();
  for (BluetoothAdapter::DeviceList::const_iterator iter = devices.begin();
       iter != devices.end();
       ++iter) {
    const BluetoothDevice* device = *iter;
    DCHECK(device);

    bluetooth::Device extension_device;
    bluetooth::BluetoothDeviceToApiDevice(*device, &extension_device);

    device_list->Append(extension_device.ToValue().release());
  }

  SendResponse(true);

  return true;
}

BluetoothGetDeviceFunction::~BluetoothGetDeviceFunction() {}

bool BluetoothGetDeviceFunction::DoWork(
    scoped_refptr<BluetoothAdapter> adapter) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  scoped_ptr<GetDevice::Params> params(GetDevice::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get() != NULL);

  BluetoothDevice* device = adapter->GetDevice(params->device_address);
  if (device) {
    bluetooth::Device extension_device;
    bluetooth::BluetoothDeviceToApiDevice(*device, &extension_device);
    SetResult(extension_device.ToValue().release());
    SendResponse(true);
  } else {
    SetError(kInvalidDevice);
    SendResponse(false);
  }

  return false;
}

void BluetoothStartDiscoveryFunction::OnSuccessCallback() {
  SendResponse(true);
}

void BluetoothStartDiscoveryFunction::OnErrorCallback() {
  SetError(kStartDiscoveryFailed);
  SendResponse(false);
}

bool BluetoothStartDiscoveryFunction::DoWork(
    scoped_refptr<BluetoothAdapter> adapter) {
  GetEventRouter(browser_context())->StartDiscoverySession(
      adapter,
      extension_id(),
      base::Bind(&BluetoothStartDiscoveryFunction::OnSuccessCallback, this),
      base::Bind(&BluetoothStartDiscoveryFunction::OnErrorCallback, this));

  return true;
}

void BluetoothStopDiscoveryFunction::OnSuccessCallback() {
  SendResponse(true);
}

void BluetoothStopDiscoveryFunction::OnErrorCallback() {
  SetError(kStopDiscoveryFailed);
  SendResponse(false);
}

bool BluetoothStopDiscoveryFunction::DoWork(
    scoped_refptr<BluetoothAdapter> adapter) {
  GetEventRouter(browser_context())->StopDiscoverySession(
      adapter,
      extension_id(),
      base::Bind(&BluetoothStopDiscoveryFunction::OnSuccessCallback, this),
      base::Bind(&BluetoothStopDiscoveryFunction::OnErrorCallback, this));

  return true;
}

}  // namespace api
}  // namespace extensions
