// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system/pointer_device_observer.h"

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/chromeos/system/input_device_settings.h"
#include "chrome/browser/chromeos/xinput_hierarchy_changed_event_listener.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {
namespace system {
namespace {

void TouchpadExistsFileThread(bool* exists) {
  *exists = touchpad_settings::TouchpadExists();
}

void MouseExistsFileThread(bool* exists) {
  *exists = mouse_settings::MouseExists();
}

}  // namespace

PointerDeviceObserver::PointerDeviceObserver()
    : ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {
}

PointerDeviceObserver::~PointerDeviceObserver() {
  XInputHierarchyChangedEventListener::GetInstance()
      ->RemoveObserver(this);
}

void PointerDeviceObserver::Init() {
  XInputHierarchyChangedEventListener::GetInstance()
      ->AddObserver(this);
}

void PointerDeviceObserver::CheckDevices() {
  CheckMouseExists();
  CheckTouchpadExists();
}

void PointerDeviceObserver::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PointerDeviceObserver::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PointerDeviceObserver::DeviceHierarchyChanged() {
  CheckDevices();
}

void PointerDeviceObserver::CheckTouchpadExists() {
  bool* exists = new bool;
  BrowserThread::PostTaskAndReply(BrowserThread::FILE, FROM_HERE,
      base::Bind(&TouchpadExistsFileThread, exists),
      base::Bind(&PointerDeviceObserver::OnTouchpadExists,
                 weak_factory_.GetWeakPtr(),
                 base::Owned(exists)));
}

void PointerDeviceObserver::CheckMouseExists() {
  bool* exists = new bool;
  BrowserThread::PostTaskAndReply(BrowserThread::FILE, FROM_HERE,
      base::Bind(&MouseExistsFileThread, exists),
      base::Bind(&PointerDeviceObserver::OnMouseExists,
                 weak_factory_.GetWeakPtr(),
                 base::Owned(exists)));
}

void PointerDeviceObserver::OnTouchpadExists(bool* exists) {
  FOR_EACH_OBSERVER(Observer, observers_, TouchpadExists(*exists));
}

void PointerDeviceObserver::OnMouseExists(bool* exists) {
  FOR_EACH_OBSERVER(Observer, observers_, MouseExists(*exists));
}

PointerDeviceObserver::Observer::~Observer() {
}

}  // namespace system
}  // namespace chromeos
