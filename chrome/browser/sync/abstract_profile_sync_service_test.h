// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_ABSTRACT_PROFILE_SYNC_SERVICE_TEST_H_
#define CHROME_BROWSER_SYNC_ABSTRACT_PROFILE_SYNC_SERVICE_TEST_H_
#pragma once

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chrome/browser/signin/token_service.h"
#include "chrome/browser/sync/profile_sync_components_factory_mock.h"
#include "content/public/test/test_browser_thread.h"
#include "sync/internal_api/public/change_record.h"
#include "sync/internal_api/public/syncable/model_type.h"
#include "testing/gtest/include/gtest/gtest.h"

class ProfileSyncService;
class TestProfileSyncService;

namespace csync {
class TestIdFactory;
}  // namespace csync

namespace csync {
struct UserShare;
}  //  namespace csync

class ProfileSyncServiceTestHelper {
 public:
  static const std::string GetTagForType(syncable::ModelType model_type);

  static bool CreateRoot(syncable::ModelType model_type,
                         csync::UserShare* service,
                         csync::TestIdFactory* ids);

  static csync::ImmutableChangeRecordList MakeSingletonChangeRecordList(
      int64 node_id, csync::ChangeRecord::Action action);

  // Deletions must provide an EntitySpecifics for the deleted data.
  static csync::ImmutableChangeRecordList
      MakeSingletonDeletionChangeRecordList(
          int64 node_id,
          const sync_pb::EntitySpecifics& specifics);
};

class AbstractProfileSyncServiceTest : public testing::Test {
 public:
  AbstractProfileSyncServiceTest();
  virtual ~AbstractProfileSyncServiceTest();

  virtual void SetUp() OVERRIDE;

  virtual void TearDown() OVERRIDE;

  bool CreateRoot(syncable::ModelType model_type);

  static ProfileKeyedService* BuildTokenService(Profile* profile);
 protected:
  MessageLoopForUI ui_loop_;
  content::TestBrowserThread ui_thread_;
  content::TestBrowserThread db_thread_;
  content::TestBrowserThread file_thread_;
  content::TestBrowserThread io_thread_;
  TokenService* token_service_;
  scoped_ptr<TestProfileSyncService> service_;
};

class CreateRootHelper {
 public:
  CreateRootHelper(AbstractProfileSyncServiceTest* test,
                   syncable::ModelType model_type);
  virtual ~CreateRootHelper();

  const base::Closure& callback() const;
  bool success();

 private:
  void CreateRootCallback();

  base::Closure callback_;
  AbstractProfileSyncServiceTest* test_;
  syncable::ModelType model_type_;
  bool success_;
};

#endif  // CHROME_BROWSER_SYNC_ABSTRACT_PROFILE_SYNC_SERVICE_TEST_H_
