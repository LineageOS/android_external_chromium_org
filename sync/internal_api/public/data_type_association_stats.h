// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_INTERNAL_API_PUBLIC_DATA_TYPE_ASSOCIATION_STATS_H_
#define SYNC_INTERNAL_API_PUBLIC_DATA_TYPE_ASSOCIATION_STATS_H_

#include "base/time.h"
#include "sync/base/sync_export.h"

namespace syncer {

// Container for datatype association results.
struct SYNC_EXPORT DataTypeAssociationStats {
  DataTypeAssociationStats();
  ~DataTypeAssociationStats();

  // The state of the world before association.
  int num_local_items_before_association;
  int num_sync_items_before_association;

  // The state of the world after association.
  int num_local_items_after_association;
  int num_sync_items_after_association;

  // The changes that took place during association. In a correctly working
  // system these should be the deltas between before and after.
  int num_local_items_added;
  int num_local_items_deleted;
  int num_local_items_modified;
  int num_sync_items_added;
  int num_sync_items_deleted;
  int num_sync_items_modified;

  // Model versions before association.
  int64 local_version_pre_association;
  int64 sync_version_pre_association;

  // Whether a datatype unrecoverable error was encountered during association.
  bool had_error;

  // Waiting time within association manager for loading local models and
  // associating other types.
  base::TimeDelta association_wait_time;

  // Time spent on association.
  base::TimeDelta association_time;
};

}  // namespace syncer

#endif  // SYNC_INTERNAL_API_PUBLIC_DATA_TYPE_ASSOCIATION_STATS_H_
