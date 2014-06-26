// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_ACTIVE_FIELD_TRIALS_H_
#define COMPONENTS_VARIATIONS_ACTIVE_FIELD_TRIALS_H_

#include <string>

#include "base/basictypes.h"
#include "base/metrics/field_trial.h"

namespace variations {

// The Unique ID of a trial and its active group, where the name and group
// identifiers are hashes of the trial and group name strings.
struct ActiveGroupId {
  uint32 name;
  uint32 group;
};

// Returns an ActiveGroupId struct for the given trial and group names.
ActiveGroupId MakeActiveGroupId(const std::string& trial_name,
                                const std::string& group_name);

// We need to supply a Compare class for templates since ActiveGroupId is a
// user-defined type.
struct ActiveGroupIdCompare {
  bool operator() (const ActiveGroupId& lhs, const ActiveGroupId& rhs) const {
    // The group and name fields are just SHA-1 Hashes, so we just need to treat
    // them as IDs and do a less-than comparison. We test group first, since
    // name is more likely to collide.
    if (lhs.group != rhs.group)
      return lhs.group < rhs.group;
    return lhs.name < rhs.name;
  }
};

// Fills the supplied vector |name_group_ids| (which must be empty when called)
// with unique ActiveGroupIds for each Field Trial that has a chosen group.
// Field Trials for which a group has not been chosen yet are NOT returned in
// this list.
void GetFieldTrialActiveGroupIds(std::vector<ActiveGroupId>* name_group_ids);

// Fills the supplied vector |output| (which must be empty when called) with
// unique string representations of ActiveGroupIds for each Field Trial that
// has a chosen group. The strings are formatted as "<TrialName>-<GroupName>",
// with the names as hex strings. Field Trials for which a group has not been
// chosen yet are NOT returned in this list.
void GetFieldTrialActiveGroupIdsAsStrings(std::vector<std::string>* output);

// Expose some functions for testing. These functions just wrap functionality
// that is implemented above.
namespace testing {

void TestGetFieldTrialActiveGroupIds(
    const base::FieldTrial::ActiveGroups& active_groups,
    std::vector<ActiveGroupId>* name_group_ids);

}  // namespace testing

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_ACTIVE_FIELD_TRIALS_H_
