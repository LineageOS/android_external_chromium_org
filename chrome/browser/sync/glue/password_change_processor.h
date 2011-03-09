// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_PASSWORD_CHANGE_PROCESSOR_H_
#define CHROME_BROWSER_SYNC_GLUE_PASSWORD_CHANGE_PROCESSOR_H_
#pragma once

#include "chrome/browser/sync/glue/change_processor.h"

#include "base/basictypes.h"
#include "chrome/browser/sync/glue/sync_backend_host.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "content/common/notification_type.h"

class PasswordStore;
class MessageLoop;
class NotificationService;

namespace browser_sync {

class PasswordModelAssociator;
class UnrecoverableErrorHandler;

// This class is responsible for taking changes from the password backend and
// applying them to the sync_api 'syncable' model, and vice versa. All
// operations and use of this class are from the DB thread on Windows and Linux,
// or the password thread on Mac.
class PasswordChangeProcessor : public ChangeProcessor,
                                public NotificationObserver {
 public:
  PasswordChangeProcessor(PasswordModelAssociator* model_associator,
                          PasswordStore* password_store,
                          UnrecoverableErrorHandler* error_handler);
  virtual ~PasswordChangeProcessor();

  // NotificationObserver implementation.
  // Passwords -> sync_api model change application.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // sync_api model -> WebDataService change application.
  virtual void ApplyChangesFromSyncModel(
      const sync_api::BaseTransaction* trans,
      const sync_api::SyncManager::ChangeRecord* changes,
      int change_count);

 protected:
  virtual void StartImpl(Profile* profile);
  virtual void StopImpl();

 private:
  void StartObserving();
  void StopObserving();

  // The two models should be associated according to this ModelAssociator.
  PasswordModelAssociator* model_associator_;

  // The model we are processing changes from.  This is owned by the
  // WebDataService which is kept alive by our data type controller
  // holding a reference.
  PasswordStore* password_store_;

  NotificationRegistrar notification_registrar_;

  bool observing_;

  MessageLoop* expected_loop_;

  DISALLOW_COPY_AND_ASSIGN(PasswordChangeProcessor);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_PASSWORD_CHANGE_PROCESSOR_H_
