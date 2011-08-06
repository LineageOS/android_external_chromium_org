// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for the SyncApi. Note that a lot of the underlying
// functionality is provided by the Syncable layer, which has its own
// unit tests. We'll test SyncApi specific things in this harness.

#include <cstddef>
#include <map>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/format_macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/scoped_temp_dir.h"
#include "base/string_number_conversions.h"
#include "base/stringprintf.h"
#include "base/tracked.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/password_manager/encryptor.h"
#include "chrome/browser/sync/engine/http_post_provider_factory.h"
#include "chrome/browser/sync/engine/http_post_provider_interface.h"
#include "chrome/browser/sync/engine/model_safe_worker.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/js_arg_list.h"
#include "chrome/browser/sync/js_backend.h"
#include "chrome/browser/sync/js_event_handler.h"
#include "chrome/browser/sync/js_reply_handler.h"
#include "chrome/browser/sync/js_test_util.h"
#include "chrome/browser/sync/notifier/sync_notifier.h"
#include "chrome/browser/sync/notifier/sync_notifier_observer.h"
#include "chrome/browser/sync/protocol/bookmark_specifics.pb.h"
#include "chrome/browser/sync/protocol/password_specifics.pb.h"
#include "chrome/browser/sync/protocol/sync.pb.h"
#include "chrome/browser/sync/protocol/proto_value_conversions.h"
#include "chrome/browser/sync/sessions/sync_session.h"
#include "chrome/browser/sync/syncable/directory_manager.h"
#include "chrome/browser/sync/syncable/nigori_util.h"
#include "chrome/browser/sync/syncable/syncable.h"
#include "chrome/browser/sync/syncable/syncable_id.h"
#include "chrome/browser/sync/util/cryptographer.h"
#include "chrome/test/base/values_test_util.h"
#include "chrome/test/sync/engine/test_user_share.h"
#include "content/browser/browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using browser_sync::Cryptographer;
using browser_sync::HasArgsAsList;
using browser_sync::HasDetailsAsDictionary;
using browser_sync::KeyParams;
using browser_sync::JsArgList;
using browser_sync::JsBackend;
using browser_sync::JsEventHandler;
using browser_sync::JsReplyHandler;
using browser_sync::MockJsEventHandler;
using browser_sync::MockJsReplyHandler;
using browser_sync::ModelSafeRoutingInfo;
using browser_sync::ModelSafeWorker;
using browser_sync::ModelSafeWorkerRegistrar;
using browser_sync::sessions::SyncSessionSnapshot;
using browser_sync::WeakHandle;
using syncable::ModelType;
using syncable::ModelTypeSet;
using test::ExpectDictDictionaryValue;
using test::ExpectDictStringValue;
using testing::_;
using testing::AnyNumber;
using testing::AtLeast;
using testing::InSequence;
using testing::Invoke;
using testing::SaveArg;
using testing::StrictMock;

namespace sync_api {

namespace {

void ExpectInt64Value(int64 expected_value,
                      const DictionaryValue& value, const std::string& key) {
  std::string int64_str;
  EXPECT_TRUE(value.GetString(key, &int64_str));
  int64 val = 0;
  EXPECT_TRUE(base::StringToInt64(int64_str, &val));
  EXPECT_EQ(expected_value, val);
}

// Makes a non-folder child of the root node.  Returns the id of the
// newly-created node.
int64 MakeNode(UserShare* share,
               ModelType model_type,
               const std::string& client_tag) {
  WriteTransaction trans(FROM_HERE, share);
  ReadNode root_node(&trans);
  root_node.InitByRootLookup();
  WriteNode node(&trans);
  EXPECT_TRUE(node.InitUniqueByCreation(model_type, root_node, client_tag));
  node.SetIsFolder(false);
  return node.GetId();
}

// Makes a non-folder child of a non-root node. Returns the id of the
// newly-created node.
int64 MakeNodeWithParent(UserShare* share,
                         ModelType model_type,
                         const std::string& client_tag,
                         int64 parent_id) {
  WriteTransaction trans(FROM_HERE, share);
  ReadNode parent_node(&trans);
  EXPECT_TRUE(parent_node.InitByIdLookup(parent_id));
  WriteNode node(&trans);
  EXPECT_TRUE(node.InitUniqueByCreation(model_type, parent_node, client_tag));
  node.SetIsFolder(false);
  return node.GetId();
}

// Makes a folder child of a non-root node. Returns the id of the
// newly-created node.
int64 MakeFolderWithParent(UserShare* share,
                           ModelType model_type,
                           int64 parent_id,
                           BaseNode* predecessor) {
  WriteTransaction trans(FROM_HERE, share);
  ReadNode parent_node(&trans);
  EXPECT_TRUE(parent_node.InitByIdLookup(parent_id));
  WriteNode node(&trans);
  EXPECT_TRUE(node.InitByCreation(model_type, parent_node, predecessor));
  node.SetIsFolder(true);
  return node.GetId();
}

// Creates the "synced" root node for a particular datatype. We use the syncable
// methods here so that the syncer treats these nodes as if they were already
// received from the server.
int64 MakeServerNodeForType(UserShare* share,
                            ModelType model_type) {
  sync_pb::EntitySpecifics specifics;
  syncable::AddDefaultExtensionValue(model_type, &specifics);
  syncable::ScopedDirLookup dir(share->dir_manager.get(), share->name);
  EXPECT_TRUE(dir.good());
  syncable::WriteTransaction trans(FROM_HERE, syncable::UNITTEST, dir);
  // Attempt to lookup by nigori tag.
  std::string type_tag = syncable::ModelTypeToRootTag(model_type);
  syncable::Id node_id = syncable::Id::CreateFromServerId(type_tag);
  syncable::MutableEntry entry(&trans, syncable::CREATE_NEW_UPDATE_ITEM,
                               node_id);
  EXPECT_TRUE(entry.good());
  entry.Put(syncable::BASE_VERSION, 1);
  entry.Put(syncable::SERVER_VERSION, 1);
  entry.Put(syncable::IS_UNAPPLIED_UPDATE, false);
  entry.Put(syncable::SERVER_PARENT_ID, syncable::kNullId);
  entry.Put(syncable::SERVER_IS_DIR, true);
  entry.Put(syncable::IS_DIR, true);
  entry.Put(syncable::SERVER_SPECIFICS, specifics);
  entry.Put(syncable::UNIQUE_SERVER_TAG, type_tag);
  entry.Put(syncable::NON_UNIQUE_NAME, type_tag);
  entry.Put(syncable::IS_DEL, false);
  entry.Put(syncable::SPECIFICS, specifics);
  return entry.Get(syncable::META_HANDLE);
}

}  // namespace

class SyncApiTest : public testing::Test {
 public:
  virtual void SetUp() {
    test_user_share_.SetUp();
  }

  virtual void TearDown() {
    test_user_share_.TearDown();
  }

 protected:
  browser_sync::TestUserShare test_user_share_;
};

TEST_F(SyncApiTest, SanityCheckTest) {
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    EXPECT_TRUE(trans.GetWrappedTrans() != NULL);
  }
  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    EXPECT_TRUE(trans.GetWrappedTrans() != NULL);
  }
  {
    // No entries but root should exist
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    // Metahandle 1 can be root, sanity check 2
    EXPECT_FALSE(node.InitByIdLookup(2));
  }
}

TEST_F(SyncApiTest, BasicTagWrite) {
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();
    EXPECT_EQ(root_node.GetFirstChildId(), 0);
  }

  ignore_result(MakeNode(test_user_share_.user_share(),
                         syncable::BOOKMARKS, "testtag"));

  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    EXPECT_TRUE(node.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));

    ReadNode root_node(&trans);
    root_node.InitByRootLookup();
    EXPECT_NE(node.GetId(), 0);
    EXPECT_EQ(node.GetId(), root_node.GetFirstChildId());
  }
}

TEST_F(SyncApiTest, GenerateSyncableHash) {
  EXPECT_EQ("OyaXV5mEzrPS4wbogmtKvRfekAI=",
      BaseNode::GenerateSyncableHash(syncable::BOOKMARKS, "tag1"));
  EXPECT_EQ("iNFQtRFQb+IZcn1kKUJEZDDkLs4=",
      BaseNode::GenerateSyncableHash(syncable::PREFERENCES, "tag1"));
  EXPECT_EQ("gO1cPZQXaM73sHOvSA+tKCKFs58=",
      BaseNode::GenerateSyncableHash(syncable::AUTOFILL, "tag1"));

  EXPECT_EQ("A0eYIHXM1/jVwKDDp12Up20IkKY=",
      BaseNode::GenerateSyncableHash(syncable::BOOKMARKS, "tag2"));
  EXPECT_EQ("XYxkF7bhS4eItStFgiOIAU23swI=",
      BaseNode::GenerateSyncableHash(syncable::PREFERENCES, "tag2"));
  EXPECT_EQ("GFiWzo5NGhjLlN+OyCfhy28DJTQ=",
      BaseNode::GenerateSyncableHash(syncable::AUTOFILL, "tag2"));
}

TEST_F(SyncApiTest, ModelTypesSiloed) {
  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();
    EXPECT_EQ(root_node.GetFirstChildId(), 0);
  }

  ignore_result(MakeNode(test_user_share_.user_share(),
                         syncable::BOOKMARKS, "collideme"));
  ignore_result(MakeNode(test_user_share_.user_share(),
                         syncable::PREFERENCES, "collideme"));
  ignore_result(MakeNode(test_user_share_.user_share(),
                         syncable::AUTOFILL, "collideme"));

  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());

    ReadNode bookmarknode(&trans);
    EXPECT_TRUE(bookmarknode.InitByClientTagLookup(syncable::BOOKMARKS,
        "collideme"));

    ReadNode prefnode(&trans);
    EXPECT_TRUE(prefnode.InitByClientTagLookup(syncable::PREFERENCES,
        "collideme"));

    ReadNode autofillnode(&trans);
    EXPECT_TRUE(autofillnode.InitByClientTagLookup(syncable::AUTOFILL,
        "collideme"));

    EXPECT_NE(bookmarknode.GetId(), prefnode.GetId());
    EXPECT_NE(autofillnode.GetId(), prefnode.GetId());
    EXPECT_NE(bookmarknode.GetId(), autofillnode.GetId());
  }
}

TEST_F(SyncApiTest, ReadMissingTagsFails) {
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    EXPECT_FALSE(node.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));
  }
  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    WriteNode node(&trans);
    EXPECT_FALSE(node.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));
  }
}

// TODO(chron): Hook this all up to the server and write full integration tests
//              for update->undelete behavior.
TEST_F(SyncApiTest, TestDeleteBehavior) {
  int64 node_id;
  int64 folder_id;
  std::wstring test_title(L"test1");

  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();

    // we'll use this spare folder later
    WriteNode folder_node(&trans);
    EXPECT_TRUE(folder_node.InitByCreation(syncable::BOOKMARKS,
        root_node, NULL));
    folder_id = folder_node.GetId();

    WriteNode wnode(&trans);
    EXPECT_TRUE(wnode.InitUniqueByCreation(syncable::BOOKMARKS,
        root_node, "testtag"));
    wnode.SetIsFolder(false);
    wnode.SetTitle(test_title);

    node_id = wnode.GetId();
  }

  // Ensure we can delete something with a tag.
  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    WriteNode wnode(&trans);
    EXPECT_TRUE(wnode.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));
    EXPECT_FALSE(wnode.GetIsFolder());
    EXPECT_EQ(wnode.GetTitle(), test_title);

    wnode.Remove();
  }

  // Lookup of a node which was deleted should return failure,
  // but have found some data about the node.
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    EXPECT_FALSE(node.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));
    // Note that for proper function of this API this doesn't need to be
    // filled, we're checking just to make sure the DB worked in this test.
    EXPECT_EQ(node.GetTitle(), test_title);
  }

  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode folder_node(&trans);
    EXPECT_TRUE(folder_node.InitByIdLookup(folder_id));

    WriteNode wnode(&trans);
    // This will undelete the tag.
    EXPECT_TRUE(wnode.InitUniqueByCreation(syncable::BOOKMARKS,
        folder_node, "testtag"));
    EXPECT_EQ(wnode.GetIsFolder(), false);
    EXPECT_EQ(wnode.GetParentId(), folder_node.GetId());
    EXPECT_EQ(wnode.GetId(), node_id);
    EXPECT_NE(wnode.GetTitle(), test_title);  // Title should be cleared
    wnode.SetTitle(test_title);
  }

  // Now look up should work.
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    EXPECT_TRUE(node.InitByClientTagLookup(syncable::BOOKMARKS,
        "testtag"));
    EXPECT_EQ(node.GetTitle(), test_title);
    EXPECT_EQ(node.GetModelType(), syncable::BOOKMARKS);
  }
}

TEST_F(SyncApiTest, WriteAndReadPassword) {
  KeyParams params = {"localhost", "username", "passphrase"};
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    trans.GetCryptographer()->AddKey(params);
  }
  {
    WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();

    WriteNode password_node(&trans);
    EXPECT_TRUE(password_node.InitUniqueByCreation(syncable::PASSWORDS,
                                                   root_node, "foo"));
    sync_pb::PasswordSpecificsData data;
    data.set_password_value("secret");
    password_node.SetPasswordSpecifics(data);
  }
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();

    ReadNode password_node(&trans);
    EXPECT_TRUE(password_node.InitByClientTagLookup(syncable::PASSWORDS,
                                                    "foo"));
    const sync_pb::PasswordSpecificsData& data =
        password_node.GetPasswordSpecifics();
    EXPECT_EQ("secret", data.password_value());
  }
}

TEST_F(SyncApiTest, BaseNodeSetSpecifics) {
  int64 child_id = MakeNode(test_user_share_.user_share(),
                            syncable::BOOKMARKS, "testtag");
  WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
  WriteNode node(&trans);
  EXPECT_TRUE(node.InitByIdLookup(child_id));

  sync_pb::EntitySpecifics entity_specifics;
  entity_specifics.MutableExtension(sync_pb::bookmark)->
      set_url("http://www.google.com");

  EXPECT_NE(entity_specifics.SerializeAsString(),
            node.GetEntitySpecifics().SerializeAsString());
  node.SetEntitySpecifics(entity_specifics);
  EXPECT_EQ(entity_specifics.SerializeAsString(),
            node.GetEntitySpecifics().SerializeAsString());
}

TEST_F(SyncApiTest, BaseNodeSetSpecificsPreservesUnknownFields) {
  int64 child_id = MakeNode(test_user_share_.user_share(),
                            syncable::BOOKMARKS, "testtag");
  WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
  WriteNode node(&trans);
  EXPECT_TRUE(node.InitByIdLookup(child_id));
  EXPECT_TRUE(node.GetEntitySpecifics().unknown_fields().empty());

  sync_pb::EntitySpecifics entity_specifics;
  entity_specifics.MutableExtension(sync_pb::bookmark)->
      set_url("http://www.google.com");
  entity_specifics.mutable_unknown_fields()->AddFixed32(5, 100);
  node.SetEntitySpecifics(entity_specifics);
  EXPECT_FALSE(node.GetEntitySpecifics().unknown_fields().empty());

  entity_specifics.mutable_unknown_fields()->Clear();
  node.SetEntitySpecifics(entity_specifics);
  EXPECT_FALSE(node.GetEntitySpecifics().unknown_fields().empty());
}

namespace {

void CheckNodeValue(const BaseNode& node, const DictionaryValue& value,
                    bool is_detailed) {
  ExpectInt64Value(node.GetId(), value, "id");
  {
    bool is_folder = false;
    EXPECT_TRUE(value.GetBoolean("isFolder", &is_folder));
    EXPECT_EQ(node.GetIsFolder(), is_folder);
  }
  ExpectDictStringValue(WideToUTF8(node.GetTitle()), value, "title");
  {
    ModelType expected_model_type = node.GetModelType();
    std::string type_str;
    EXPECT_TRUE(value.GetString("type", &type_str));
    if (expected_model_type >= syncable::FIRST_REAL_MODEL_TYPE) {
      ModelType model_type =
          syncable::ModelTypeFromString(type_str);
      EXPECT_EQ(expected_model_type, model_type);
    } else if (expected_model_type == syncable::TOP_LEVEL_FOLDER) {
      EXPECT_EQ("Top-level folder", type_str);
    } else if (expected_model_type == syncable::UNSPECIFIED) {
      EXPECT_EQ("Unspecified", type_str);
    } else {
      ADD_FAILURE();
    }
  }
  if (is_detailed) {
    ExpectInt64Value(node.GetParentId(), value, "parentId");
    ExpectInt64Value(node.GetModificationTime(), value, "modificationTime");
    ExpectInt64Value(node.GetExternalId(), value, "externalId");
    ExpectInt64Value(node.GetPredecessorId(), value, "predecessorId");
    ExpectInt64Value(node.GetSuccessorId(), value, "successorId");
    ExpectInt64Value(node.GetFirstChildId(), value, "firstChildId");
    {
      scoped_ptr<DictionaryValue> expected_entry(node.GetEntry()->ToValue());
      Value* entry = NULL;
      EXPECT_TRUE(value.Get("entry", &entry));
      EXPECT_TRUE(Value::Equals(entry, expected_entry.get()));
    }
    EXPECT_EQ(11u, value.size());
  } else {
    EXPECT_EQ(4u, value.size());
  }
}

}  // namespace

TEST_F(SyncApiTest, BaseNodeGetSummaryAsValue) {
  ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
  ReadNode node(&trans);
  node.InitByRootLookup();
  scoped_ptr<DictionaryValue> details(node.GetSummaryAsValue());
  if (details.get()) {
    CheckNodeValue(node, *details, false);
  } else {
    ADD_FAILURE();
  }
}

TEST_F(SyncApiTest, BaseNodeGetDetailsAsValue) {
  ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
  ReadNode node(&trans);
  node.InitByRootLookup();
  scoped_ptr<DictionaryValue> details(node.GetDetailsAsValue());
  if (details.get()) {
    CheckNodeValue(node, *details, true);
  } else {
    ADD_FAILURE();
  }
}

namespace {

void ExpectChangeRecordActionValue(SyncManager::ChangeRecord::Action
                                       expected_value,
                                   const DictionaryValue& value,
                                   const std::string& key) {
  std::string str_value;
  EXPECT_TRUE(value.GetString(key, &str_value));
  switch (expected_value) {
    case SyncManager::ChangeRecord::ACTION_ADD:
      EXPECT_EQ("Add", str_value);
      break;
    case SyncManager::ChangeRecord::ACTION_UPDATE:
      EXPECT_EQ("Update", str_value);
      break;
    case SyncManager::ChangeRecord::ACTION_DELETE:
      EXPECT_EQ("Delete", str_value);
      break;
    default:
      NOTREACHED();
      break;
  }
}

void CheckNonDeleteChangeRecordValue(const SyncManager::ChangeRecord& record,
                                     const DictionaryValue& value,
                                     BaseTransaction* trans) {
  EXPECT_NE(SyncManager::ChangeRecord::ACTION_DELETE, record.action);
  ExpectChangeRecordActionValue(record.action, value, "action");
  {
    ReadNode node(trans);
    EXPECT_TRUE(node.InitByIdLookup(record.id));
    scoped_ptr<DictionaryValue> expected_details(node.GetDetailsAsValue());
    ExpectDictDictionaryValue(*expected_details, value, "node");
  }
}

void CheckDeleteChangeRecordValue(const SyncManager::ChangeRecord& record,
                                  const DictionaryValue& value) {
  EXPECT_EQ(SyncManager::ChangeRecord::ACTION_DELETE, record.action);
  ExpectChangeRecordActionValue(record.action, value, "action");
  DictionaryValue* node_value = NULL;
  EXPECT_TRUE(value.GetDictionary("node", &node_value));
  if (node_value) {
    ExpectInt64Value(record.id, *node_value, "id");
    scoped_ptr<DictionaryValue> expected_specifics_value(
        browser_sync::EntitySpecificsToValue(record.specifics));
    ExpectDictDictionaryValue(*expected_specifics_value,
                              *node_value, "specifics");
    scoped_ptr<DictionaryValue> expected_extra_value;
    if (record.extra.get()) {
      expected_extra_value.reset(record.extra->ToValue());
    }
    Value* extra_value = NULL;
    EXPECT_EQ(record.extra.get() != NULL,
              node_value->Get("extra", &extra_value));
    EXPECT_TRUE(Value::Equals(extra_value, expected_extra_value.get()));
  }
}

class MockExtraChangeRecordData
    : public SyncManager::ExtraPasswordChangeRecordData {
 public:
  MOCK_CONST_METHOD0(ToValue, DictionaryValue*());
};

}  // namespace

TEST_F(SyncApiTest, ChangeRecordToValue) {
  int64 child_id = MakeNode(test_user_share_.user_share(),
                            syncable::BOOKMARKS, "testtag");
  sync_pb::EntitySpecifics child_specifics;
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ReadNode node(&trans);
    EXPECT_TRUE(node.InitByIdLookup(child_id));
    child_specifics = node.GetEntry()->Get(syncable::SPECIFICS);
  }

  // Add
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    SyncManager::ChangeRecord record;
    record.action = SyncManager::ChangeRecord::ACTION_ADD;
    record.id = 1;
    record.specifics = child_specifics;
    record.extra.reset(new StrictMock<MockExtraChangeRecordData>());
    scoped_ptr<DictionaryValue> value(record.ToValue(&trans));
    CheckNonDeleteChangeRecordValue(record, *value, &trans);
  }

  // Update
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    SyncManager::ChangeRecord record;
    record.action = SyncManager::ChangeRecord::ACTION_UPDATE;
    record.id = child_id;
    record.specifics = child_specifics;
    record.extra.reset(new StrictMock<MockExtraChangeRecordData>());
    scoped_ptr<DictionaryValue> value(record.ToValue(&trans));
    CheckNonDeleteChangeRecordValue(record, *value, &trans);
  }

  // Delete (no extra)
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    SyncManager::ChangeRecord record;
    record.action = SyncManager::ChangeRecord::ACTION_DELETE;
    record.id = child_id + 1;
    record.specifics = child_specifics;
    scoped_ptr<DictionaryValue> value(record.ToValue(&trans));
    CheckDeleteChangeRecordValue(record, *value);
  }

  // Delete (with extra)
  {
    ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    SyncManager::ChangeRecord record;
    record.action = SyncManager::ChangeRecord::ACTION_DELETE;
    record.id = child_id + 1;
    record.specifics = child_specifics;

    DictionaryValue extra_value;
    extra_value.SetString("foo", "bar");
    scoped_ptr<StrictMock<MockExtraChangeRecordData> > extra(
        new StrictMock<MockExtraChangeRecordData>());
    EXPECT_CALL(*extra, ToValue()).Times(2).WillRepeatedly(
        Invoke(&extra_value, &DictionaryValue::DeepCopy));

    record.extra.reset(extra.release());
    scoped_ptr<DictionaryValue> value(record.ToValue(&trans));
    CheckDeleteChangeRecordValue(record, *value);
  }
}

namespace {

class TestHttpPostProviderInterface : public HttpPostProviderInterface {
 public:
  virtual ~TestHttpPostProviderInterface() {}

  virtual void SetUserAgent(const char* user_agent) OVERRIDE {}
  virtual void SetExtraRequestHeaders(const char* headers) OVERRIDE {}
  virtual void SetURL(const char* url, int port) OVERRIDE {}
  virtual void SetPostPayload(const char* content_type,
                              int content_length,
                              const char* content) OVERRIDE {}
  virtual bool MakeSynchronousPost(int* os_error_code, int* response_code)
      OVERRIDE {
    return false;
  }
  virtual int GetResponseContentLength() const OVERRIDE {
    return 0;
  }
  virtual const char* GetResponseContent() const OVERRIDE {
    return "";
  }
  virtual const std::string GetResponseHeaderValue(
      const std::string& name) const OVERRIDE {
    return "";
  }
  virtual void Abort() OVERRIDE {}
};

class TestHttpPostProviderFactory : public HttpPostProviderFactory {
 public:
  virtual ~TestHttpPostProviderFactory() {}
  virtual HttpPostProviderInterface* Create() OVERRIDE {
    return new TestHttpPostProviderInterface();
  }
  virtual void Destroy(HttpPostProviderInterface* http) OVERRIDE {
    delete http;
  }
};

class SyncManagerObserverMock : public SyncManager::Observer {
 public:
  MOCK_METHOD4(OnChangesApplied,
               void(ModelType,
                    const BaseTransaction*,
                    const SyncManager::ChangeRecord*,
                    int));  // NOLINT
  MOCK_METHOD1(OnChangesComplete, void(ModelType));  // NOLINT
  MOCK_METHOD1(OnSyncCycleCompleted,
               void(const SyncSessionSnapshot*));  // NOLINT
  MOCK_METHOD1(OnInitializationComplete,
               void(const WeakHandle<JsBackend>&));  // NOLINT
  MOCK_METHOD1(OnAuthError, void(const GoogleServiceAuthError&));  // NOLINT
  MOCK_METHOD1(OnPassphraseRequired,
               void(sync_api::PassphraseRequiredReason));  // NOLINT
  MOCK_METHOD1(OnPassphraseAccepted, void(const std::string&));  // NOLINT
  MOCK_METHOD0(OnStopSyncingPermanently, void());  // NOLINT
  MOCK_METHOD1(OnUpdatedToken, void(const std::string&));  // NOLINT
  MOCK_METHOD1(OnMigrationNeededForTypes, void(const ModelTypeSet&));
  MOCK_METHOD0(OnClearServerDataFailed, void());  // NOLINT
  MOCK_METHOD0(OnClearServerDataSucceeded, void());  // NOLINT
  MOCK_METHOD1(OnEncryptionComplete, void(const ModelTypeSet&));  // NOLINT
};

class SyncNotifierMock : public sync_notifier::SyncNotifier {
 public:
  MOCK_METHOD1(AddObserver, void(sync_notifier::SyncNotifierObserver*));
  MOCK_METHOD1(RemoveObserver, void(sync_notifier::SyncNotifierObserver*));
  MOCK_METHOD1(SetUniqueId, void(const std::string&));
  MOCK_METHOD1(SetState, void(const std::string&));
  MOCK_METHOD2(UpdateCredentials,
               void(const std::string&, const std::string&));
  MOCK_METHOD1(UpdateEnabledTypes,
               void(const syncable::ModelTypeSet&));
  MOCK_METHOD0(SendNotification, void());
};

class SyncManagerTest : public testing::Test,
                        public ModelSafeWorkerRegistrar {
 protected:
  SyncManagerTest()
      : ui_thread_(BrowserThread::UI, &ui_loop_),
        sync_notifier_mock_(NULL),
        sync_manager_("Test sync manager"),
        sync_notifier_observer_(NULL),
        update_enabled_types_call_count_(0) {}

  virtual ~SyncManagerTest() {
    EXPECT_FALSE(sync_notifier_mock_);
  }

  // Test implementation.
  void SetUp() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    SyncCredentials credentials;
    credentials.email = "foo@bar.com";
    credentials.sync_token = "sometoken";

    sync_notifier_mock_ = new StrictMock<SyncNotifierMock>();
    EXPECT_CALL(*sync_notifier_mock_, AddObserver(_)).
        WillOnce(Invoke(this, &SyncManagerTest::SyncNotifierAddObserver));
    EXPECT_CALL(*sync_notifier_mock_, SetUniqueId(_));
    EXPECT_CALL(*sync_notifier_mock_, SetState(""));
    EXPECT_CALL(*sync_notifier_mock_,
                UpdateCredentials(credentials.email, credentials.sync_token));
    EXPECT_CALL(*sync_notifier_mock_, UpdateEnabledTypes(_)).
        Times(AtLeast(1)).
        WillRepeatedly(
            Invoke(this, &SyncManagerTest::SyncNotifierUpdateEnabledTypes));
    EXPECT_CALL(*sync_notifier_mock_, RemoveObserver(_)).
        WillOnce(Invoke(this, &SyncManagerTest::SyncNotifierRemoveObserver));

    sync_manager_.AddObserver(&observer_);
    EXPECT_CALL(observer_, OnInitializationComplete(_)).
        WillOnce(SaveArg<0>(&js_backend_));

    EXPECT_FALSE(sync_notifier_observer_);
    EXPECT_FALSE(js_backend_.IsInitialized());

    // Takes ownership of |sync_notifier_mock_|.
    sync_manager_.Init(temp_dir_.path(),
                       WeakHandle<JsEventHandler>(),
                       "bogus", 0, false,
                       new TestHttpPostProviderFactory(), this, "bogus",
                       credentials, sync_notifier_mock_, "",
                       true /* setup_for_test_mode */);

    EXPECT_TRUE(sync_notifier_observer_);
    EXPECT_TRUE(js_backend_.IsInitialized());

    EXPECT_EQ(1, update_enabled_types_call_count_);

    ModelSafeRoutingInfo routes;
    GetModelSafeRoutingInfo(&routes);
    for (ModelSafeRoutingInfo::iterator i = routes.begin(); i != routes.end();
         ++i) {
      EXPECT_CALL(observer_, OnChangesApplied(i->first, _, _, 1))
          .RetiresOnSaturation();
      EXPECT_CALL(observer_, OnChangesComplete(i->first))
          .RetiresOnSaturation();
      type_roots_[i->first] = MakeServerNodeForType(
          sync_manager_.GetUserShare(), i->first);
    }
    PumpLoop();
  }

  void TearDown() {
    sync_manager_.RemoveObserver(&observer_);
    sync_manager_.Shutdown();
    sync_notifier_mock_ = NULL;
    EXPECT_FALSE(sync_notifier_observer_);
    PumpLoop();
  }

  // ModelSafeWorkerRegistrar implementation.
  virtual void GetWorkers(std::vector<ModelSafeWorker*>* out) {
    NOTIMPLEMENTED();
    out->clear();
  }
  virtual void GetModelSafeRoutingInfo(ModelSafeRoutingInfo* out) {
    (*out)[syncable::NIGORI] = browser_sync::GROUP_PASSIVE;
    (*out)[syncable::BOOKMARKS] = browser_sync::GROUP_PASSIVE;
    (*out)[syncable::THEMES] = browser_sync::GROUP_PASSIVE;
    (*out)[syncable::SESSIONS] = browser_sync::GROUP_PASSIVE;
    (*out)[syncable::PASSWORDS] = browser_sync::GROUP_PASSIVE;
  }

  // Helper methods.
  bool SetUpEncryption() {
    // Mock the Mac Keychain service. The real Keychain can block on user input.
    #if defined(OS_MACOSX)
      Encryptor::UseMockKeychain(true);
    #endif

    // We need to create the nigori node as if it were an applied server update.
    UserShare* share = sync_manager_.GetUserShare();
    int64 nigori_id = GetIdForDataType(syncable::NIGORI);
    if (nigori_id == kInvalidId)
      return false;

    // Set the nigori cryptographer information.
    WriteTransaction trans(FROM_HERE, share);
    Cryptographer* cryptographer = trans.GetCryptographer();
    if (!cryptographer)
      return false;
    KeyParams params = {"localhost", "dummy", "foobar"};
    cryptographer->AddKey(params);
    sync_pb::NigoriSpecifics nigori;
    cryptographer->GetKeys(nigori.mutable_encrypted());
    WriteNode node(&trans);
    EXPECT_TRUE(node.InitByIdLookup(nigori_id));
    node.SetNigoriSpecifics(nigori);
    return cryptographer->is_ready();
  }

  int64 GetIdForDataType(ModelType type) {
    if (type_roots_.count(type) == 0)
      return 0;
    return type_roots_[type];
  }

  void SyncNotifierAddObserver(
      sync_notifier::SyncNotifierObserver* sync_notifier_observer) {
    EXPECT_EQ(NULL, sync_notifier_observer_);
    sync_notifier_observer_ = sync_notifier_observer;
  }

  void SyncNotifierRemoveObserver(
      sync_notifier::SyncNotifierObserver* sync_notifier_observer) {
    EXPECT_EQ(sync_notifier_observer_, sync_notifier_observer);
    sync_notifier_observer_ = NULL;
  }

  void SyncNotifierUpdateEnabledTypes(
      const syncable::ModelTypeSet& types) {
    ModelSafeRoutingInfo routes;
    GetModelSafeRoutingInfo(&routes);
    syncable::ModelTypeSet expected_types;
    for (ModelSafeRoutingInfo::const_iterator it = routes.begin();
         it != routes.end(); ++it) {
      expected_types.insert(it->first);
    }
    EXPECT_EQ(expected_types, types);
    ++update_enabled_types_call_count_;
  }

  void PumpLoop() {
    ui_loop_.RunAllPending();
  }

  void SendJsMessage(const std::string& name, const JsArgList& args,
                     const WeakHandle<JsReplyHandler>& reply_handler) {
    js_backend_.Call(FROM_HERE, &JsBackend::ProcessJsMessage,
                     name, args, reply_handler);
    PumpLoop();
  }

  void SetJsEventHandler(const WeakHandle<JsEventHandler>& event_handler) {
    js_backend_.Call(FROM_HERE, &JsBackend::SetJsEventHandler,
                     event_handler);
    PumpLoop();
  }

 private:
  // Needed by |ui_thread_|.
  MessageLoopForUI ui_loop_;
  // Needed by |sync_manager_|.
  BrowserThread ui_thread_;
  // Needed by |sync_manager_|.
  ScopedTempDir temp_dir_;
  // Sync Id's for the roots of the enabled datatypes.
  std::map<ModelType, int64> type_roots_;
  StrictMock<SyncNotifierMock>* sync_notifier_mock_;

 protected:
  SyncManager sync_manager_;
  WeakHandle<JsBackend> js_backend_;
  StrictMock<SyncManagerObserverMock> observer_;
  sync_notifier::SyncNotifierObserver* sync_notifier_observer_;
  int update_enabled_types_call_count_;
};

TEST_F(SyncManagerTest, UpdateEnabledTypes) {
  EXPECT_EQ(1, update_enabled_types_call_count_);
  // Triggers SyncNotifierUpdateEnabledTypes.
  sync_manager_.UpdateEnabledTypes();
  EXPECT_EQ(2, update_enabled_types_call_count_);
}

TEST_F(SyncManagerTest, ProcessJsMessage) {
  const JsArgList kNoArgs;

  StrictMock<MockJsReplyHandler> reply_handler;

  ListValue false_args;
  false_args.Append(Value::CreateBooleanValue(false));

  EXPECT_CALL(reply_handler,
              HandleJsReply("getNotificationState",
                            HasArgsAsList(false_args)));

  // This message should be dropped.
  SendJsMessage("unknownMessage", kNoArgs, reply_handler.AsWeakHandle());

  SendJsMessage("getNotificationState", kNoArgs, reply_handler.AsWeakHandle());
}

TEST_F(SyncManagerTest, ProcessJsMessageGetRootNodeDetails) {
  const JsArgList kNoArgs;

  StrictMock<MockJsReplyHandler> reply_handler;

  JsArgList return_args;

  EXPECT_CALL(reply_handler,
              HandleJsReply("getRootNodeDetails", _))
      .WillOnce(SaveArg<1>(&return_args));

  SendJsMessage("getRootNodeDetails", kNoArgs, reply_handler.AsWeakHandle());

  EXPECT_EQ(1u, return_args.Get().GetSize());
  DictionaryValue* node_info = NULL;
  EXPECT_TRUE(return_args.Get().GetDictionary(0, &node_info));
  if (node_info) {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode node(&trans);
    node.InitByRootLookup();
    CheckNodeValue(node, *node_info, true);
  } else {
    ADD_FAILURE();
  }
}

void CheckGetNodesByIdReturnArgs(const SyncManager& sync_manager,
                                 const JsArgList& return_args,
                                 int64 id,
                                 bool is_detailed) {
  EXPECT_EQ(1u, return_args.Get().GetSize());
  ListValue* nodes = NULL;
  ASSERT_TRUE(return_args.Get().GetList(0, &nodes));
  ASSERT_TRUE(nodes);
  EXPECT_EQ(1u, nodes->GetSize());
  DictionaryValue* node_info = NULL;
  EXPECT_TRUE(nodes->GetDictionary(0, &node_info));
  ASSERT_TRUE(node_info);
  ReadTransaction trans(FROM_HERE, sync_manager.GetUserShare());
  ReadNode node(&trans);
  EXPECT_TRUE(node.InitByIdLookup(id));
  CheckNodeValue(node, *node_info, is_detailed);
}

class SyncManagerGetNodesByIdTest : public SyncManagerTest {
 protected:
  virtual ~SyncManagerGetNodesByIdTest() {}

  void RunGetNodesByIdTest(const char* message_name, bool is_detailed) {
    int64 root_id = kInvalidId;
    {
      ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
      ReadNode root_node(&trans);
      root_node.InitByRootLookup();
      root_id = root_node.GetId();
    }

    int64 child_id =
        MakeNode(sync_manager_.GetUserShare(),
                 syncable::BOOKMARKS, "testtag");

    StrictMock<MockJsReplyHandler> reply_handler;

    JsArgList return_args;

    const int64 ids[] = { root_id, child_id };

    EXPECT_CALL(reply_handler,
                HandleJsReply(message_name, _))
        .Times(arraysize(ids)).WillRepeatedly(SaveArg<1>(&return_args));

    for (size_t i = 0; i < arraysize(ids); ++i) {
      ListValue args;
      ListValue* id_values = new ListValue();
      args.Append(id_values);
      id_values->Append(Value::CreateStringValue(base::Int64ToString(ids[i])));
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());

      CheckGetNodesByIdReturnArgs(sync_manager_, return_args,
                                  ids[i], is_detailed);
    }
  }

  void RunGetNodesByIdFailureTest(const char* message_name) {
    StrictMock<MockJsReplyHandler> reply_handler;

    ListValue empty_list_args;
    empty_list_args.Append(new ListValue());

    EXPECT_CALL(reply_handler,
                HandleJsReply(message_name,
                                    HasArgsAsList(empty_list_args)))
        .Times(6);

    {
      ListValue args;
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }

    {
      ListValue args;
      args.Append(new ListValue());
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }

    {
      ListValue args;
      ListValue* ids = new ListValue();
      args.Append(ids);
      ids->Append(Value::CreateStringValue(""));
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }

    {
      ListValue args;
      ListValue* ids = new ListValue();
      args.Append(ids);
      ids->Append(Value::CreateStringValue("nonsense"));
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }

    {
      ListValue args;
      ListValue* ids = new ListValue();
      args.Append(ids);
      ids->Append(Value::CreateStringValue("0"));
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }

    {
      ListValue args;
      ListValue* ids = new ListValue();
      args.Append(ids);
      ids->Append(Value::CreateStringValue("9999"));
      SendJsMessage(message_name,
                    JsArgList(&args), reply_handler.AsWeakHandle());
    }
  }
};

TEST_F(SyncManagerGetNodesByIdTest, GetNodeSummariesById) {
  RunGetNodesByIdTest("getNodeSummariesById", false);
}

TEST_F(SyncManagerGetNodesByIdTest, GetNodeDetailsById) {
  RunGetNodesByIdTest("getNodeDetailsById", true);
}

TEST_F(SyncManagerGetNodesByIdTest, GetNodeSummariesByIdFailure) {
  RunGetNodesByIdFailureTest("getNodeSummariesById");
}

TEST_F(SyncManagerGetNodesByIdTest, GetNodeDetailsByIdFailure) {
  RunGetNodesByIdFailureTest("getNodeDetailsById");
}

TEST_F(SyncManagerTest, GetChildNodeIds) {
  StrictMock<MockJsReplyHandler> reply_handler;

  JsArgList return_args;

  EXPECT_CALL(reply_handler,
              HandleJsReply("getChildNodeIds", _))
      .Times(1).WillRepeatedly(SaveArg<1>(&return_args));

  {
    ListValue args;
    args.Append(Value::CreateStringValue("1"));
    SendJsMessage("getChildNodeIds",
                  JsArgList(&args), reply_handler.AsWeakHandle());
  }

  EXPECT_EQ(1u, return_args.Get().GetSize());
  ListValue* nodes = NULL;
  ASSERT_TRUE(return_args.Get().GetList(0, &nodes));
  ASSERT_TRUE(nodes);
  EXPECT_EQ(5u, nodes->GetSize());
}

TEST_F(SyncManagerTest, GetChildNodeIdsFailure) {
  StrictMock<MockJsReplyHandler> reply_handler;

  ListValue empty_list_args;
  empty_list_args.Append(new ListValue());

  EXPECT_CALL(reply_handler,
              HandleJsReply("getChildNodeIds",
                                   HasArgsAsList(empty_list_args)))
      .Times(5);

  {
    ListValue args;
    SendJsMessage("getChildNodeIds",
                   JsArgList(&args), reply_handler.AsWeakHandle());
  }

  {
    ListValue args;
    args.Append(Value::CreateStringValue(""));
    SendJsMessage("getChildNodeIds",
                  JsArgList(&args), reply_handler.AsWeakHandle());
  }

  {
    ListValue args;
    args.Append(Value::CreateStringValue("nonsense"));
    SendJsMessage("getChildNodeIds",
                  JsArgList(&args), reply_handler.AsWeakHandle());
  }

  {
    ListValue args;
    args.Append(Value::CreateStringValue("0"));
    SendJsMessage("getChildNodeIds",
                  JsArgList(&args), reply_handler.AsWeakHandle());
  }

  {
    ListValue args;
    args.Append(Value::CreateStringValue("9999"));
    SendJsMessage("getChildNodeIds",
                  JsArgList(&args), reply_handler.AsWeakHandle());
  }
}

// TODO(akalin): Add unit tests for findNodesContainingString message.

TEST_F(SyncManagerTest, OnNotificationStateChange) {
  InSequence dummy;
  StrictMock<MockJsEventHandler> event_handler;

  DictionaryValue true_details;
  true_details.SetBoolean("enabled", true);
  DictionaryValue false_details;
  false_details.SetBoolean("enabled", false);

  EXPECT_CALL(event_handler,
              HandleJsEvent("onNotificationStateChange",
                            HasDetailsAsDictionary(true_details)));
  EXPECT_CALL(event_handler,
              HandleJsEvent("onNotificationStateChange",
                            HasDetailsAsDictionary(false_details)));

  sync_manager_.TriggerOnNotificationStateChangeForTest(true);
  sync_manager_.TriggerOnNotificationStateChangeForTest(false);

  SetJsEventHandler(event_handler.AsWeakHandle());
  sync_manager_.TriggerOnNotificationStateChangeForTest(true);
  sync_manager_.TriggerOnNotificationStateChangeForTest(false);
  SetJsEventHandler(WeakHandle<JsEventHandler>());

  sync_manager_.TriggerOnNotificationStateChangeForTest(true);
  sync_manager_.TriggerOnNotificationStateChangeForTest(false);

  // Should trigger the replies.
  PumpLoop();
}

TEST_F(SyncManagerTest, OnIncomingNotification) {
  StrictMock<MockJsEventHandler> event_handler;

  const syncable::ModelTypeBitSet empty_model_types;
  syncable::ModelTypeBitSet model_types;
  model_types.set(syncable::BOOKMARKS);
  model_types.set(syncable::THEMES);

  // Build expected_args to have a single argument with the string
  // equivalents of model_types.
  DictionaryValue expected_details;
  {
    ListValue* model_type_list = new ListValue();
    expected_details.Set("changedTypes", model_type_list);
    for (int i = syncable::FIRST_REAL_MODEL_TYPE;
         i < syncable::MODEL_TYPE_COUNT; ++i) {
      if (model_types[i]) {
        model_type_list->Append(
            Value::CreateStringValue(
                syncable::ModelTypeToString(
                    syncable::ModelTypeFromInt(i))));
      }
    }
  }

  EXPECT_CALL(event_handler,
              HandleJsEvent("onIncomingNotification",
                           HasDetailsAsDictionary(expected_details)));

  sync_manager_.TriggerOnIncomingNotificationForTest(empty_model_types);
  sync_manager_.TriggerOnIncomingNotificationForTest(model_types);

  SetJsEventHandler(event_handler.AsWeakHandle());
  sync_manager_.TriggerOnIncomingNotificationForTest(model_types);
  SetJsEventHandler(WeakHandle<JsEventHandler>());

  sync_manager_.TriggerOnIncomingNotificationForTest(empty_model_types);
  sync_manager_.TriggerOnIncomingNotificationForTest(model_types);

  // Should trigger the replies.
  PumpLoop();
}

TEST_F(SyncManagerTest, RefreshEncryptionReady) {
  EXPECT_TRUE(SetUpEncryption());
  sync_manager_.RefreshEncryption();
  syncable::ModelTypeSet encrypted_types =
      sync_manager_.GetEncryptedDataTypes();
  EXPECT_EQ(1U, encrypted_types.count(syncable::PASSWORDS));
}

// Attempt to refresh encryption when nigori not downloaded.
TEST_F(SyncManagerTest, RefreshEncryptionNotReady) {
  // Don't set up encryption (no nigori node created).
  sync_manager_.RefreshEncryption();  // Should fail.
  syncable::ModelTypeSet encrypted_types =
      sync_manager_.GetEncryptedDataTypes();
  EXPECT_EQ(1U, encrypted_types.count(syncable::PASSWORDS));  // Hardcoded.
}

TEST_F(SyncManagerTest, EncryptDataTypesWithNoData) {
  EXPECT_TRUE(SetUpEncryption());
  ModelTypeSet encrypted_types;
  encrypted_types.insert(syncable::BOOKMARKS);
  // Even though Passwords isn't marked for encryption, it's enabled, so it
  // should automatically be added to the response of OnEncryptionComplete.
  ModelTypeSet expected_types = encrypted_types;
  expected_types.insert(syncable::PASSWORDS);
  EXPECT_CALL(observer_, OnEncryptionComplete(expected_types));
  sync_manager_.EncryptDataTypes(encrypted_types);
  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    EXPECT_EQ(expected_types,
              GetEncryptedTypes(&trans));
  }
}

TEST_F(SyncManagerTest, EncryptDataTypesWithData) {
  size_t batch_size = 5;
  EXPECT_TRUE(SetUpEncryption());

  // Create some unencrypted unsynced data.
  int64 folder = MakeFolderWithParent(sync_manager_.GetUserShare(),
                                      syncable::BOOKMARKS,
                                      GetIdForDataType(syncable::BOOKMARKS),
                                      NULL);
  // First batch_size nodes are children of folder.
  size_t i;
  for (i = 0; i < batch_size; ++i) {
    MakeNodeWithParent(sync_manager_.GetUserShare(), syncable::BOOKMARKS,
                       base::StringPrintf("%"PRIuS"", i), folder);
  }
  // Next batch_size nodes are a different type and on their own.
  for (; i < 2*batch_size; ++i) {
    MakeNodeWithParent(sync_manager_.GetUserShare(), syncable::SESSIONS,
                       base::StringPrintf("%"PRIuS"", i),
                       GetIdForDataType(syncable::SESSIONS));
  }
  // Last batch_size nodes are a third type that will not need encryption.
  for (; i < 3*batch_size; ++i) {
    MakeNodeWithParent(sync_manager_.GetUserShare(), syncable::THEMES,
                       base::StringPrintf("%"PRIuS"", i),
                       GetIdForDataType(syncable::THEMES));
  }

  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::BOOKMARKS,
                                                   false /* not encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::SESSIONS,
                                                   false /* not encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::THEMES,
                                                   false /* not encrypted */));
  }

  ModelTypeSet encrypted_types;
  encrypted_types.insert(syncable::BOOKMARKS);
  encrypted_types.insert(syncable::SESSIONS);
  encrypted_types.insert(syncable::PASSWORDS);
  EXPECT_CALL(observer_, OnEncryptionComplete(encrypted_types));
  sync_manager_.EncryptDataTypes(encrypted_types);

  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    EXPECT_EQ(encrypted_types, GetEncryptedTypes(&trans));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::BOOKMARKS,
                                                   true /* is encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::SESSIONS,
                                                   true /* is encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::THEMES,
                                                   false /* not encrypted */));
  }

  // Trigger's a ReEncryptEverything with new passphrase.
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_CALL(observer_, OnPassphraseAccepted(_)).Times(1);
  EXPECT_CALL(observer_, OnEncryptionComplete(encrypted_types)).Times(1);
  sync_manager_.SetPassphrase("new_passphrase", true);
  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    EXPECT_EQ(encrypted_types, GetEncryptedTypes(&trans));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::BOOKMARKS,
                                                   true /* is encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::SESSIONS,
                                                   true /* is encrypted */));
    EXPECT_TRUE(syncable::VerifyDataTypeEncryption(trans.GetWrappedTrans(),
                                                   trans.GetCryptographer(),
                                                   syncable::THEMES,
                                                   false /* not encrypted */));
  }
  // Calling EncryptDataTypes with an empty encrypted types should not trigger
  // a reencryption and should just notify immediately.
  // TODO(zea): add logic to ensure nothing was written.
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_CALL(observer_, OnPassphraseAccepted(_)).Times(0);
  EXPECT_CALL(observer_, OnEncryptionComplete(encrypted_types)).Times(1);
  sync_manager_.EncryptDataTypes(encrypted_types);
}

TEST_F(SyncManagerTest, SetPassphraseWithPassword) {
  EXPECT_TRUE(SetUpEncryption());
  {
    WriteTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();

    WriteNode password_node(&trans);
    EXPECT_TRUE(password_node.InitUniqueByCreation(syncable::PASSWORDS,
                                                   root_node, "foo"));
    sync_pb::PasswordSpecificsData data;
    data.set_password_value("secret");
    password_node.SetPasswordSpecifics(data);
  }
  EXPECT_CALL(observer_, OnPassphraseAccepted(_));
  EXPECT_CALL(observer_, OnEncryptionComplete(_));
  sync_manager_.SetPassphrase("new_passphrase", true);
  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode password_node(&trans);
    EXPECT_TRUE(password_node.InitByClientTagLookup(syncable::PASSWORDS,
                                                    "foo"));
    const sync_pb::PasswordSpecificsData& data =
        password_node.GetPasswordSpecifics();
    EXPECT_EQ("secret", data.password_value());
  }
}

TEST_F(SyncManagerTest, SetPassphraseWithEmptyPasswordNode) {
  EXPECT_TRUE(SetUpEncryption());
  int64 node_id = 0;
  std::string tag = "foo";
  {
    WriteTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode root_node(&trans);
    root_node.InitByRootLookup();

    WriteNode password_node(&trans);
    EXPECT_TRUE(password_node.InitUniqueByCreation(syncable::PASSWORDS,
                                                   root_node, tag));
    node_id = password_node.GetId();
  }
  EXPECT_CALL(observer_, OnPassphraseAccepted(_));
  EXPECT_CALL(observer_, OnEncryptionComplete(_));
  sync_manager_.SetPassphrase("new_passphrase", true);
  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode password_node(&trans);
    EXPECT_FALSE(password_node.InitByClientTagLookup(syncable::PASSWORDS,
                                                     tag));
  }
  {
    ReadTransaction trans(FROM_HERE, sync_manager_.GetUserShare());
    ReadNode password_node(&trans);
    EXPECT_FALSE(password_node.InitByIdLookup(node_id));
  }
}

}  // namespace

}  // namespace browser_sync
