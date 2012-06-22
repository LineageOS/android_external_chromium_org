// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Defines ChangeReorderBuffer, which can be used to sort a list of item
// actions to achieve the ordering constraint required by the SyncObserver
// interface of the SyncAPI.

#ifndef SYNC_INTERNAL_API_CHANGE_REORDER_BUFFER_H_
#define SYNC_INTERNAL_API_CHANGE_REORDER_BUFFER_H_
#pragma once

#include <map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/linked_ptr.h"
#include "sync/internal_api/public/base_transaction.h"
#include "sync/internal_api/public/change_record.h"
#include "sync/protocol/sync.pb.h"

namespace csync {

// ChangeReorderBuffer is a utility type which accepts an unordered set
// of changes (via its Push methods), and yields an ImmutableChangeRecordList
// (via the GetAllChangesInTreeOrder method) that are in the order that
// the SyncObserver expects them to be. A buffer is initially empty.
//
// The ordering produced by ChangeReorderBuffer is as follows:
//  (a) All Deleted items appear first.
//  (b) For Updated and/or Added items, parents appear before their children.
//  (c) When there are changes to the sibling order (this means Added items,
//      or Updated items with the |position_changed| parameter set to true),
//      all siblings under a parent will appear in the output, even if they
//      are not explicitly pushed.  The sibling order will be preserved in
//      the output list -- items will appear before their sibling-order
//      successors.
//  (d) When there are no changes to the sibling order under a parent node,
//      the sibling order is not necessarily preserved in the output for
//      its children.
class ChangeReorderBuffer {
 public:
  ChangeReorderBuffer();
  ~ChangeReorderBuffer();

  // Insert an item, identified by the metahandle |id|, into the reorder
  // buffer. This item will appear in the output list as an ACTION_ADD
  // ChangeRecord.
  void PushAddedItem(int64 id) {
    operations_[id] = OP_ADD;
  }

  // Insert an item, identified by the metahandle |id|, into the reorder
  // buffer. This item will appear in the output list as an ACTION_DELETE
  // ChangeRecord.
  void PushDeletedItem(int64 id) {
    operations_[id] = OP_DELETE;
  }

  // Insert an item, identified by the metahandle |id|, into the reorder
  // buffer. This item will appear in the output list as an ACTION_UPDATE
  // ChangeRecord. Also, if |position_changed| is true, all siblings of this
  // item will appear in the output list as well; if it wasn't explicitly
  // pushed, the siblings will have an ACTION_UPDATE ChangeRecord.
  void PushUpdatedItem(int64 id, bool position_changed) {
    operations_[id] = position_changed ? OP_UPDATE_POSITION_AND_PROPERTIES :
                                         OP_UPDATE_PROPERTIES_ONLY;
  }

  void SetExtraDataForId(int64 id, ExtraPasswordChangeRecordData* extra) {
    extra_data_[id] = make_linked_ptr<ExtraPasswordChangeRecordData>(extra);
  }

  void SetSpecificsForId(int64 id, const sync_pb::EntitySpecifics& specifics) {
    specifics_[id] = specifics;
  }

  // Reset the buffer, forgetting any pushed items, so that it can be used
  // again to reorder a new set of changes.
  void Clear() {
    operations_.clear();
  }

  bool IsEmpty() const {
    return operations_.empty();
  }

  // Output a reordered list of changes to |changes| using the items
  // that were pushed into the reorder buffer. |sync_trans| is used to
  // determine the ordering.  Returns true if successful, or false if
  // an error was encountered.
  bool GetAllChangesInTreeOrder(
      const BaseTransaction* sync_trans,
      ImmutableChangeRecordList* changes) WARN_UNUSED_RESULT;

 private:
  class Traversal;
  enum Operation {
    OP_ADD,                             // AddedItem.
    OP_DELETE,                          // DeletedItem.
    OP_UPDATE_PROPERTIES_ONLY,          // UpdatedItem with position_changed=0.
    OP_UPDATE_POSITION_AND_PROPERTIES,  // UpdatedItem with position_changed=1.
  };
  typedef std::map<int64, Operation> OperationMap;
  typedef std::map<int64, sync_pb::EntitySpecifics> SpecificsMap;
  typedef std::map<int64, linked_ptr<ExtraPasswordChangeRecordData> >
      ExtraDataMap;

  // Stores the items that have been pushed into the buffer, and the type of
  // operation that was associated with them.
  OperationMap operations_;

  // Stores entity-specific ChangeRecord data per-ID.
  SpecificsMap specifics_;

  // Stores type-specific extra data per-ID.
  ExtraDataMap extra_data_;

  DISALLOW_COPY_AND_ASSIGN(ChangeReorderBuffer);
};

}  // namespace csync

#endif  // SYNC_INTERNAL_API_CHANGE_REORDER_BUFFER_H_
