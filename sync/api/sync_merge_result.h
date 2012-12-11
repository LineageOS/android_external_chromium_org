// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_API_SYNC_MERGE_RESULT_H_
#define SYNC_API_SYNC_MERGE_RESULT_H_

#include "sync/api/sync_error.h"
#include "sync/base/sync_export.h"
#include "sync/internal_api/public/base/model_type.h"

namespace syncer {

// A model-type-specific view of a sync merge. This class encapsulates the
// state before and after the merge as well as the deltas and any error that
// occurred.
// Note: This class only tracks one side of the merge. In other words, if built
// by the local SyncableService, all values correspond to the local state before
// and after merging, and the delta's applied to that state. Sync's change
// processor will create a separate merge result.
class SYNC_EXPORT SyncMergeResult {
 public:
  // Initialize an empty merge result for model type |type|.
  explicit SyncMergeResult(ModelType type);
  ~SyncMergeResult();

  // Default copy and assign welcome.

  // Setters.
  // Note: if |error.IsSet()| is true, |error.type()| must match model_type_
  void set_error(SyncError error);
  void set_num_items_before_association(int num_items_before_association);
  void set_num_items_after_association(int num_items_after_association);
  void set_num_items_added(int num_items_added);
  void set_num_items_deleted(int num_items_deleted);
  void set_num_items_modified(int num_items_modified);

  // Getters.
  ModelType model_type() const;
  SyncError error() const;
  int num_items_before_association() const;
  int num_items_after_association() const;
  int num_items_added() const;
  int num_items_deleted() const;
  int num_items_modified() const;

 private:
  // Make |this| into a copy of |other|.
  void CopyFrom(const SyncMergeResult& other);

  // The datatype that was associated.
  ModelType model_type_;

  // The error encountered during association. Unset if no error was
  // encountered.
  SyncError error_;

  // The state of the world before association.
  int num_items_before_association_;

  // The state of the world after association.
  int num_items_after_association_;

  // The changes that took place during association. In a correctly working
  // system these should be the deltas between before and after.
  int num_items_added_;
  int num_items_deleted_;
  int num_items_modified_;
};

}  // namespace syncer

#endif  // SYNC_API_SYNC_MERGE_RESULT_H_
