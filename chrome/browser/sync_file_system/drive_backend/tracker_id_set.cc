// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/tracker_id_set.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/sync_file_system/drive_backend/metadata_database.pb.h"

namespace sync_file_system {
namespace drive_backend {

TrackerIDSet::TrackerIDSet() : active_tracker_(0) {}

TrackerIDSet::~TrackerIDSet() {}

void TrackerIDSet::Insert(const FileTracker& tracker) {
  if (tracker.active())
    InsertActiveTracker(tracker.tracker_id());
  else
    InsertInactiveTracker(tracker.tracker_id());
}

void TrackerIDSet::InsertActiveTracker(int64 tracker_id) {
  DCHECK(tracker_id);
  DCHECK(!active_tracker_);

  active_tracker_ = tracker_id;
  tracker_ids_.insert(tracker_id);
}

void TrackerIDSet::InsertInactiveTracker(int64 tracker_id) {
  DCHECK(tracker_id);
  DCHECK_NE(active_tracker_, tracker_id);

  tracker_ids_.insert(tracker_id);
}

void TrackerIDSet::Erase(int64 tracker_id) {
  DCHECK(ContainsKey(tracker_ids_, tracker_id));

  if (active_tracker_ == tracker_id)
    active_tracker_ = 0;
  tracker_ids_.erase(tracker_id);
}

void TrackerIDSet::Activate(int64 tracker_id) {
  DCHECK(!active_tracker_);
  DCHECK(ContainsKey(tracker_ids_, tracker_id));
  active_tracker_ = tracker_id;
}

void TrackerIDSet::Deactivate(int64 tracker_id) {
  DCHECK_EQ(active_tracker_, tracker_id);
  DCHECK(ContainsKey(tracker_ids_, tracker_id));
  active_tracker_ = 0;
}

}  // namespace drive_backend
}  // namespace sync_file_system
