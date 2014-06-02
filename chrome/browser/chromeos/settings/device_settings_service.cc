// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_settings_service.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/owner_key_util.h"
#include "chrome/browser/chromeos/settings/session_manager_operation.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "crypto/rsa_private_key.h"

namespace em = enterprise_management;

namespace {

// Delay between load retries when there was a validation error.
// NOTE: This code is here to mitigate clock loss on some devices where policy
// loads will fail with a validation error caused by RTC clock bing reset when
// the battery is drained.
int kLoadRetryDelayMs = 1000 * 5;
// Maximal number of retries before we give up. Calculated to allow for 10 min
// of retry time.
int kMaxLoadRetries = (1000 * 60 * 10) / kLoadRetryDelayMs;

}  // namespace

namespace chromeos {

OwnerKey::OwnerKey(scoped_ptr<std::vector<uint8> > public_key,
                   scoped_ptr<crypto::RSAPrivateKey> private_key)
    : public_key_(public_key.Pass()),
      private_key_(private_key.Pass()) {}

OwnerKey::~OwnerKey() {}

DeviceSettingsService::Observer::~Observer() {}

static DeviceSettingsService* g_device_settings_service = NULL;

// static
void DeviceSettingsService::Initialize() {
  CHECK(!g_device_settings_service);
  g_device_settings_service = new DeviceSettingsService();
}

// static
bool DeviceSettingsService::IsInitialized() {
  return g_device_settings_service;
}

// static
void DeviceSettingsService::Shutdown() {
  DCHECK(g_device_settings_service);
  delete g_device_settings_service;
  g_device_settings_service = NULL;
}

// static
DeviceSettingsService* DeviceSettingsService::Get() {
  CHECK(g_device_settings_service);
  return g_device_settings_service;
}

DeviceSettingsService::DeviceSettingsService()
    : session_manager_client_(NULL),
      store_status_(STORE_SUCCESS),
      waiting_for_tpm_token_(true),
      owner_key_loaded_with_tpm_token_(false),
      load_retries_left_(kMaxLoadRetries),
      weak_factory_(this) {
  if (TPMTokenLoader::IsInitialized()) {
    waiting_for_tpm_token_ = !TPMTokenLoader::Get()->IsTPMTokenReady();
    TPMTokenLoader::Get()->AddObserver(this);
  }
}

DeviceSettingsService::~DeviceSettingsService() {
  DCHECK(pending_operations_.empty());
  if (TPMTokenLoader::IsInitialized())
    TPMTokenLoader::Get()->RemoveObserver(this);
}

void DeviceSettingsService::SetSessionManager(
    SessionManagerClient* session_manager_client,
    scoped_refptr<OwnerKeyUtil> owner_key_util) {
  DCHECK(session_manager_client);
  DCHECK(owner_key_util.get());
  DCHECK(!session_manager_client_);
  DCHECK(!owner_key_util_.get());

  session_manager_client_ = session_manager_client;
  owner_key_util_ = owner_key_util;

  session_manager_client_->AddObserver(this);

  StartNextOperation();
}

void DeviceSettingsService::UnsetSessionManager() {
  STLDeleteContainerPointers(pending_operations_.begin(),
                             pending_operations_.end());
  pending_operations_.clear();

  if (session_manager_client_)
    session_manager_client_->RemoveObserver(this);
  session_manager_client_ = NULL;
  owner_key_util_ = NULL;
}

scoped_refptr<OwnerKey> DeviceSettingsService::GetOwnerKey() {
  return owner_key_;
}

void DeviceSettingsService::Load() {
  EnqueueLoad(false);
}

void DeviceSettingsService::SignAndStore(
    scoped_ptr<em::ChromeDeviceSettingsProto> new_settings,
    const base::Closure& callback) {
  scoped_ptr<em::PolicyData> new_policy = AssemblePolicy(*new_settings);
  if (!new_policy) {
    HandleError(STORE_POLICY_ERROR, callback);
    return;
  }

  Enqueue(
      new SignAndStoreSettingsOperation(
          base::Bind(&DeviceSettingsService::HandleCompletedOperation,
                     weak_factory_.GetWeakPtr(),
                     callback),
          new_policy.Pass()));
}

void DeviceSettingsService::SetManagementSettings(
    em::PolicyData::ManagementMode management_mode,
    const std::string& request_token,
    const std::string& device_id,
    const base::Closure& callback) {
  if (!CheckManagementModeTransition(management_mode)) {
    LOG(ERROR) << "Invalid management mode transition: current mode = "
               << GetManagementMode() << ", new mode = " << management_mode;
    HandleError(STORE_POLICY_ERROR, callback);
    return;
  }

  scoped_ptr<em::PolicyData> policy = AssemblePolicy(*device_settings_);
  if (!policy) {
    HandleError(STORE_POLICY_ERROR, callback);
    return;
  }

  policy->set_management_mode(management_mode);
  policy->set_request_token(request_token);
  policy->set_device_id(device_id);

  Enqueue(
      new SignAndStoreSettingsOperation(
          base::Bind(&DeviceSettingsService::HandleCompletedOperation,
                     weak_factory_.GetWeakPtr(),
                     callback),
          policy.Pass()));
}

void DeviceSettingsService::Store(scoped_ptr<em::PolicyFetchResponse> policy,
                                  const base::Closure& callback) {
  Enqueue(
      new StoreSettingsOperation(
          base::Bind(&DeviceSettingsService::HandleCompletedOperation,
                     weak_factory_.GetWeakPtr(),
                     callback),
          policy.Pass()));
}

DeviceSettingsService::OwnershipStatus
    DeviceSettingsService::GetOwnershipStatus() {
  if (owner_key_.get())
    return owner_key_->public_key() ? OWNERSHIP_TAKEN : OWNERSHIP_NONE;

  return OWNERSHIP_UNKNOWN;
}

void DeviceSettingsService::GetOwnershipStatusAsync(
    const OwnershipStatusCallback& callback) {
  if (owner_key_.get()) {
    // If there is a key, report status immediately.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(
            callback,
            owner_key_->public_key() ? OWNERSHIP_TAKEN : OWNERSHIP_NONE));
  } else {
    // If the key hasn't been loaded yet, enqueue the callback to be fired when
    // the next SessionManagerOperation completes. If no operation is pending,
    // start a load operation to fetch the key and report the result.
    pending_ownership_status_callbacks_.push_back(callback);
    if (pending_operations_.empty())
      EnqueueLoad(false);
  }
}

bool DeviceSettingsService::HasPrivateOwnerKey() {
  return owner_key_.get() && owner_key_->private_key();
}

void DeviceSettingsService::IsCurrentUserOwnerAsync(
    const IsCurrentUserOwnerCallback& callback) {
  if (owner_key_loaded_with_tpm_token_) {
    // If the current owner key was loaded while the certificates were loaded,
    // or the certificate loader is not initialized, in which case the private
    // key cannot be set, report status immediately.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, HasPrivateOwnerKey()));
  } else {
    // If the key hasn't been loaded with the known certificates, enqueue the
    // callback to be fired when the next SessionManagerOperation completes in
    // an environment where the certificates are loaded. There is no need to
    // start a new operation, as the reload operation will be started when the
    // certificates are loaded.
    pending_is_current_user_owner_callbacks_.push_back(callback);
  }
}

void DeviceSettingsService::InitOwner(const std::string& username,
                                      crypto::ScopedPK11Slot slot) {
  if (!username_.empty())
    return;

  username_ = username;
  slot_ = slot.Pass();

  // The private key may have become available, so force a key reload.
  owner_key_ = NULL;
  EnsureReload(true);
}

const std::string& DeviceSettingsService::GetUsername() const {
  return username_;
}

void DeviceSettingsService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceSettingsService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void DeviceSettingsService::OwnerKeySet(bool success) {
  if (!success) {
    LOG(ERROR) << "Owner key change failed.";
    return;
  }

  owner_key_ = NULL;
  EnsureReload(true);
}

void DeviceSettingsService::PropertyChangeComplete(bool success) {
  if (!success) {
    LOG(ERROR) << "Policy update failed.";
    return;
  }

  EnsureReload(false);
}

void DeviceSettingsService::OnTPMTokenReady() {
  waiting_for_tpm_token_ = false;

  // TPMTokenLoader initializes the TPM and NSS database which is necessary to
  // determine ownership. Force a reload once we know these are initialized.
  EnsureReload(true);
}

void DeviceSettingsService::Enqueue(SessionManagerOperation* operation) {
  pending_operations_.push_back(operation);
  if (pending_operations_.front() == operation)
    StartNextOperation();
}

void DeviceSettingsService::EnqueueLoad(bool force_key_load) {
  SessionManagerOperation* operation =
      new LoadSettingsOperation(
          base::Bind(&DeviceSettingsService::HandleCompletedOperation,
                     weak_factory_.GetWeakPtr(),
                     base::Closure()));
  operation->set_force_key_load(force_key_load);
  operation->set_username(username_);
  operation->set_slot(slot_.get());
  Enqueue(operation);
}

void DeviceSettingsService::EnsureReload(bool force_key_load) {
  if (!pending_operations_.empty()) {
    pending_operations_.front()->set_username(username_);
    pending_operations_.front()->set_slot(slot_.get());
    pending_operations_.front()->RestartLoad(force_key_load);
  } else {
    EnqueueLoad(force_key_load);
  }
}

void DeviceSettingsService::StartNextOperation() {
  if (!pending_operations_.empty() &&
      session_manager_client_ &&
      owner_key_util_.get()) {
    pending_operations_.front()->Start(session_manager_client_,
                                       owner_key_util_, owner_key_);
  }
}

void DeviceSettingsService::HandleCompletedOperation(
    const base::Closure& callback,
    SessionManagerOperation* operation,
    Status status) {
  DCHECK_EQ(operation, pending_operations_.front());
  store_status_ = status;

  OwnershipStatus ownership_status = OWNERSHIP_UNKNOWN;
  bool is_owner = false;
  scoped_refptr<OwnerKey> new_key(operation->owner_key());
  if (new_key.get()) {
    ownership_status =
        new_key->public_key() ? OWNERSHIP_TAKEN : OWNERSHIP_NONE;
    is_owner = (new_key->private_key() != NULL);
  } else {
    NOTREACHED() << "Failed to determine key status.";
  }

  bool new_owner_key = false;
  if (owner_key_.get() != new_key.get()) {
    owner_key_ = new_key;
    new_owner_key = true;
  }

  if (status == STORE_SUCCESS) {
    policy_data_ = operation->policy_data().Pass();
    device_settings_ = operation->device_settings().Pass();
    load_retries_left_ = kMaxLoadRetries;
  } else if (status != STORE_KEY_UNAVAILABLE) {
    LOG(ERROR) << "Session manager operation failed: " << status;
    // Validation errors can be temporary if the rtc has gone on holiday for a
    // short while. So we will retry such loads for up to 10 minutes.
    if (status == STORE_TEMP_VALIDATION_ERROR) {
      if (load_retries_left_ > 0) {
        load_retries_left_--;
        LOG(ERROR) << "A re-load has been scheduled due to a validation error.";
        content::BrowserThread::PostDelayedTask(
            content::BrowserThread::UI,
            FROM_HERE,
            base::Bind(&DeviceSettingsService::Load, base::Unretained(this)),
            base::TimeDelta::FromMilliseconds(kLoadRetryDelayMs));
      } else {
        // Once we've given up retrying, the validation error is not temporary
        // anymore.
        store_status_ = STORE_VALIDATION_ERROR;
      }
    }
  }

  if (new_owner_key) {
    FOR_EACH_OBSERVER(Observer, observers_, OwnershipStatusChanged());
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_OWNERSHIP_STATUS_CHANGED,
        content::Source<DeviceSettingsService>(this),
        content::NotificationService::NoDetails());
  }

  FOR_EACH_OBSERVER(Observer, observers_, DeviceSettingsUpdated());

  std::vector<OwnershipStatusCallback> callbacks;
  callbacks.swap(pending_ownership_status_callbacks_);
  for (std::vector<OwnershipStatusCallback>::iterator iter(callbacks.begin());
       iter != callbacks.end(); ++iter) {
    iter->Run(ownership_status);
  }

  if (!waiting_for_tpm_token_) {
    owner_key_loaded_with_tpm_token_ = true;
    std::vector<IsCurrentUserOwnerCallback> is_owner_callbacks;
    is_owner_callbacks.swap(pending_is_current_user_owner_callbacks_);
    for (std::vector<IsCurrentUserOwnerCallback>::iterator iter(
             is_owner_callbacks.begin());
         iter != is_owner_callbacks.end(); ++iter) {
      iter->Run(is_owner);
    }
  }

  // The completion callback happens after the notification so clients can
  // filter self-triggered updates.
  if (!callback.is_null())
    callback.Run();

  // Only remove the pending operation here, so new operations triggered by any
  // of the callbacks above are queued up properly.
  pending_operations_.pop_front();
  delete operation;

  StartNextOperation();
}

void DeviceSettingsService::HandleError(Status status,
                                        const base::Closure& callback) {
  store_status_ = status;

  LOG(ERROR) << "Session manager operation failed: " << status;

  FOR_EACH_OBSERVER(Observer, observers_, DeviceSettingsUpdated());

  // The completion callback happens after the notification so clients can
  // filter self-triggered updates.
  if (!callback.is_null())
    callback.Run();
}

scoped_ptr<em::PolicyData> DeviceSettingsService::AssemblePolicy(
    const em::ChromeDeviceSettingsProto& settings) const {
  scoped_ptr<em::PolicyData> policy(new em::PolicyData());
  if (policy_data_) {
    // Preserve management settings.
    if (policy_data_->has_management_mode())
      policy->set_management_mode(policy_data_->management_mode());
    if (policy_data_->has_request_token())
      policy->set_request_token(policy_data_->request_token());
    if (policy_data_->has_device_id())
      policy->set_device_id(policy_data_->device_id());
  } else {
    // If there's no previous policy data, this is the first time the device
    // setting is set. We set the management mode to NOT_MANAGED initially.
    policy->set_management_mode(em::PolicyData::NOT_MANAGED);
  }
  policy->set_policy_type(policy::dm_protocol::kChromeDevicePolicyType);
  policy->set_timestamp((base::Time::Now() - base::Time::UnixEpoch()).
                        InMilliseconds());
  policy->set_username(username_);
  if (!settings.SerializeToString(policy->mutable_policy_value()))
    return scoped_ptr<em::PolicyData>();

  return policy.Pass();
}

em::PolicyData::ManagementMode DeviceSettingsService::GetManagementMode()
    const {
  if (policy_data_ && policy_data_->has_management_mode())
    return policy_data_->management_mode();
  return em::PolicyData::NOT_MANAGED;
}

bool DeviceSettingsService::CheckManagementModeTransition(
    em::PolicyData::ManagementMode new_mode) const {
  em::PolicyData::ManagementMode current_mode = GetManagementMode();

  // Mode is not changed.
  if (current_mode == new_mode)
    return true;

  switch (current_mode) {
    case em::PolicyData::NOT_MANAGED:
      // For consumer management enrollment.
      return new_mode == em::PolicyData::CONSUMER_MANAGED;

    case em::PolicyData::ENTERPRISE_MANAGED:
      // Management mode cannot be set when it is currently ENTERPRISE_MANAGED.
      return false;

    case em::PolicyData::CONSUMER_MANAGED:
      // For consumer management unenrollment.
      return new_mode == em::PolicyData::NOT_MANAGED;
  }

  NOTREACHED();
  return false;
}

ScopedTestDeviceSettingsService::ScopedTestDeviceSettingsService() {
  DeviceSettingsService::Initialize();
}

ScopedTestDeviceSettingsService::~ScopedTestDeviceSettingsService() {
  // Clean pending operations.
  DeviceSettingsService::Get()->UnsetSessionManager();
  DeviceSettingsService::Shutdown();
}

}  // namespace chromeos
