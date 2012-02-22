// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/dom_storage/dom_storage_map.h"

#include "base/logging.h"

namespace {

size_t size_of_item(const string16& key, const string16& value) {
  return (key.length() + value.length()) * sizeof(char16);
}

size_t CountBytes(const dom_storage::ValuesMap& values) {
  if (values.size() == 0)
    return 0;

  size_t count = 0;
  dom_storage::ValuesMap::const_iterator it = values.begin();
  for (; it != values.end(); ++it)
    count += size_of_item(it->first, it->second.string());
  return count;
}

}  // namespace

namespace dom_storage {

DomStorageMap::DomStorageMap(size_t quota)
    : bytes_used_(0),
      quota_(quota) {
  ResetKeyIterator();
}

DomStorageMap::~DomStorageMap() {}

unsigned DomStorageMap::Length() const {
  return values_.size();
}

NullableString16 DomStorageMap::Key(unsigned index) {
  if (index >= values_.size())
    return NullableString16(true);
  while (last_key_index_ != index) {
    if (last_key_index_ > index) {
      --key_iterator_;
      --last_key_index_;
    } else {
      ++key_iterator_;
      ++last_key_index_;
    }
  }
  return NullableString16(key_iterator_->first, false);
}

NullableString16 DomStorageMap::GetItem(const string16& key) const {
  ValuesMap::const_iterator found = values_.find(key);
  if (found == values_.end())
    return NullableString16(true);
  return found->second;
}

bool DomStorageMap::SetItem(
    const string16& key, const string16& value,
    NullableString16* old_value) {
  ValuesMap::const_iterator found = values_.find(key);
  if (found == values_.end())
    *old_value = NullableString16(true);
  else
    *old_value = found->second;

  size_t old_item_size = old_value->is_null() ?
      0 : size_of_item(key, old_value->string());
  size_t new_size = bytes_used_ - old_item_size + size_of_item(key, value);
  if (new_size > quota_)
    return false;

  values_[key] = NullableString16(value, false);
  ResetKeyIterator();
  bytes_used_ -= old_item_size;
  bytes_used_ += size_of_item(key, value);
  return true;
}

bool DomStorageMap::RemoveItem(
    const string16& key,
    string16* old_value) {
  ValuesMap::iterator found = values_.find(key);
  if (found == values_.end())
    return false;
  *old_value = found->second.string();
  values_.erase(found);
  ResetKeyIterator();
  bytes_used_ -= size_of_item(key, *old_value);
  return true;
}

bool DomStorageMap::SwapValues(ValuesMap* values) {
  size_t new_size = CountBytes(*values);
  if (new_size > quota_)
    return false;
  values_.swap(*values);
  bytes_used_ = new_size;
  ResetKeyIterator();
  return true;
}

DomStorageMap* DomStorageMap::DeepCopy() const {
  DCHECK(CountBytes(values_) <= quota_);
  DomStorageMap* copy = new DomStorageMap(quota_);
  copy->values_ = values_;
  copy->bytes_used_ = bytes_used_;
  copy->ResetKeyIterator();
  return copy;
}

void DomStorageMap::ResetKeyIterator() {
  key_iterator_ = values_.begin();
  last_key_index_ = 0;
}

}  // namespace dom_storage
