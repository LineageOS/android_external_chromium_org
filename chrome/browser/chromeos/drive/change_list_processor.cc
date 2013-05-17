// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/change_list_processor.h"

#include <utility>

#include "base/metrics/histogram.h"
#include "chrome/browser/chromeos/drive/drive.pb.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/drive/resource_entry_conversion.h"
#include "chrome/browser/chromeos/drive/resource_metadata.h"
#include "chrome/browser/google_apis/drive_api_parser.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace drive {

namespace {

// Callback for ResourceMetadata::SetLargestChangestamp.
// Runs |on_complete_callback|. |on_complete_callback| must not be null.
void RunOnCompleteCallback(const base::Closure& on_complete_callback,
                           FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!on_complete_callback.is_null());
  DLOG_IF(ERROR, error != FILE_ERROR_OK) << "SetLargestChangeStamp failed: "
                                         << FileErrorToString(error);
  on_complete_callback.Run();
}

}  // namespace

ChangeList::ChangeList(const google_apis::ResourceList& resource_list)
    : largest_changestamp_(resource_list.largest_changestamp()) {
  resource_list.GetNextFeedURL(&next_url_);

  entries_.resize(resource_list.entries().size());
  for (size_t i = 0; i < resource_list.entries().size(); ++i) {
    ConvertToResourceEntry(*resource_list.entries()[i]).Swap(
        &entries_[i]);
  }
}

ChangeList::~ChangeList() {}

class ChangeListProcessor::ChangeListToEntryProtoMapUMAStats {
 public:
  ChangeListToEntryProtoMapUMAStats()
    : num_regular_files_(0),
      num_hosted_documents_(0),
      num_shared_with_me_entries_(0) {
  }

  // Increments number of files.
  void IncrementNumFiles(bool is_hosted_document) {
    is_hosted_document ? num_hosted_documents_++ : num_regular_files_++;
  }

  // Increments number of shared-with-me entries.
  void IncrementNumSharedWithMeEntries() {
    num_shared_with_me_entries_++;
  }

  // Updates UMA histograms with file counts.
  void UpdateFileCountUmaHistograms() {
    const int num_total_files = num_hosted_documents_ + num_regular_files_;
    UMA_HISTOGRAM_COUNTS("Drive.NumberOfRegularFiles", num_regular_files_);
    UMA_HISTOGRAM_COUNTS("Drive.NumberOfHostedDocuments",
                         num_hosted_documents_);
    UMA_HISTOGRAM_COUNTS("Drive.NumberOfTotalFiles", num_total_files);
    UMA_HISTOGRAM_COUNTS("Drive.NumberOfSharedWithMeEntries",
                         num_shared_with_me_entries_);
  }

 private:
  int num_regular_files_;
  int num_hosted_documents_;
  int num_shared_with_me_entries_;
};

ChangeListProcessor::ChangeListProcessor(
    internal::ResourceMetadata* resource_metadata)
  : resource_metadata_(resource_metadata),
    largest_changestamp_(0),
    weak_ptr_factory_(this) {
}

ChangeListProcessor::~ChangeListProcessor() {
}

void ChangeListProcessor::ApplyFeeds(
    scoped_ptr<google_apis::AboutResource> about_resource,
    ScopedVector<ChangeList> change_lists,
    bool is_delta_feed,
    const base::Closure& on_complete_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!on_complete_callback.is_null());
  DCHECK(is_delta_feed || about_resource.get());

  Clear();

  on_complete_callback_ = on_complete_callback;
  largest_changestamp_ = 0;
  if (is_delta_feed) {
    if (!change_lists.empty()) {
      // The changestamp appears in the first page of the change list.
      // The changestamp does not appear in the full resource list.
      largest_changestamp_ = change_lists[0]->largest_changestamp();
      DCHECK_GE(change_lists[0]->largest_changestamp(), 0);
    }
  } else if (about_resource.get()) {
    largest_changestamp_ = about_resource->largest_change_id();

    DVLOG(1) << "Root folder ID is " << about_resource->root_folder_id();
    DCHECK(!about_resource->root_folder_id().empty());
  } else {
    // A full update without AboutResouce will have no effective changestamp.
    NOTREACHED();
  }

  ChangeListToEntryProtoMapUMAStats uma_stats;
  FeedToEntryMap(change_lists.Pass(), &entry_map_, &uma_stats);
  ApplyEntryProtoMap(is_delta_feed, about_resource.Pass());

  // Shouldn't record histograms when processing delta feeds.
  if (!is_delta_feed)
    uma_stats.UpdateFileCountUmaHistograms();
}

void ChangeListProcessor::ApplyEntryProtoMap(
    bool is_delta_feed,
    scoped_ptr<google_apis::AboutResource> about_resource) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_delta_feed) {  // Full update.
    resource_metadata_->ResetOnUIThread(
        base::Bind(&ChangeListProcessor::ApplyEntryProtoMapAfterReset,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Passed(&about_resource)));
  } else {
    // Go through all entries generated by the feed and apply them to the local
    // snapshot of the file system.
    ApplyNextEntryProtoAsync();
  }
}

void ChangeListProcessor::ApplyEntryProtoMapAfterReset(
    scoped_ptr<google_apis::AboutResource> about_resource,
    FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(about_resource);

  LOG_IF(ERROR, error != FILE_ERROR_OK) << "Failed to reset: "
                                        << FileErrorToString(error);

  changed_dirs_.insert(util::GetDriveGrandRootPath());
  changed_dirs_.insert(util::GetDriveMyDriveRootPath());

  // Create the MyDrive root directory.
  ApplyEntryProto(util::CreateMyDriveRootEntry(
      about_resource->root_folder_id()));
}

void ChangeListProcessor::ApplyNextEntryProtoAsync() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(&ChangeListProcessor::ApplyNextEntryProto,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ChangeListProcessor::ApplyNextEntryProto() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!entry_map_.empty()) {
    ApplyNextByIterator(entry_map_.begin());  // Continue.
  } else {
    // Update the root entry and finish.
    UpdateRootEntry(base::Bind(&ChangeListProcessor::OnComplete,
                               weak_ptr_factory_.GetWeakPtr()));
  }
}

void ChangeListProcessor::ApplyNextByIterator(ResourceEntryMap::iterator it) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  ResourceEntry entry = it->second;
  DCHECK_EQ(it->first, entry.resource_id());
  // Add the largest changestamp if this entry is a directory.
  if (entry.file_info().is_directory()) {
    entry.mutable_directory_specific_info()->set_changestamp(
        largest_changestamp_);
  }

  // The parent of this entry may not yet be processed. We need the parent
  // to be rooted in the metadata tree before we can add the child, so process
  // the parent first.
  ResourceEntryMap::iterator parent_it = entry_map_.find(
      entry.parent_resource_id());
  if (parent_it != entry_map_.end()) {
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(&ChangeListProcessor::ApplyNextByIterator,
                   weak_ptr_factory_.GetWeakPtr(),
                   parent_it));
  } else {
    // Erase the entry so the deleted entry won't be referenced.
    entry_map_.erase(it);
    ApplyEntryProto(entry);
  }
}

void ChangeListProcessor::ApplyEntryProto(const ResourceEntry& entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Lookup the entry.
  resource_metadata_->GetResourceEntryByIdOnUIThread(
      entry.resource_id(),
      base::Bind(&ChangeListProcessor::ContinueApplyEntryProto,
                 weak_ptr_factory_.GetWeakPtr(),
                 entry));
}

void ChangeListProcessor::ContinueApplyEntryProto(
    const ResourceEntry& entry,
    FileError error,
    const base::FilePath& file_path,
    scoped_ptr<ResourceEntry> old_entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (error == FILE_ERROR_OK) {
    if (entry.deleted()) {
      // Deleted file/directory.
      RemoveEntryFromParent(entry, file_path);
    } else {
      // Entry exists and needs to be refreshed.
      RefreshEntry(entry, file_path);
    }
  } else if (error == FILE_ERROR_NOT_FOUND && !entry.deleted()) {
    // Adding a new entry.
    AddEntry(entry);
  } else {
    // Continue.
    ApplyNextEntryProtoAsync();
  }
}

void ChangeListProcessor::AddEntry(const ResourceEntry& entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  resource_metadata_->AddEntryOnUIThread(
      entry,
      base::Bind(&ChangeListProcessor::NotifyForAddEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 entry.file_info().is_directory()));
}

void ChangeListProcessor::NotifyForAddEntry(bool is_directory,
                                            FileError error,
                                            const base::FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DVLOG(1) << "NotifyForAddEntry " << file_path.value() << ", error = "
           << FileErrorToString(error);
  if (error == FILE_ERROR_OK) {
    // Notify if a directory has been created.
    if (is_directory)
      changed_dirs_.insert(file_path);

    // Notify parent.
    changed_dirs_.insert(file_path.DirName());
  }

  ApplyNextEntryProtoAsync();
}

void ChangeListProcessor::RemoveEntryFromParent(
    const ResourceEntry& entry,
    const base::FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!file_path.empty());

  if (!entry.file_info().is_directory()) {
    // No children if entry is a file.
    OnGetChildrenForRemove(entry, file_path, std::set<base::FilePath>());
  } else {
    // If entry is a directory, notify its children.
    resource_metadata_->GetChildDirectoriesOnUIThread(
        entry.resource_id(),
        base::Bind(&ChangeListProcessor::OnGetChildrenForRemove,
                   weak_ptr_factory_.GetWeakPtr(),
                   entry,
                   file_path));
  }
}

void ChangeListProcessor::OnGetChildrenForRemove(
    const ResourceEntry& entry,
    const base::FilePath& file_path,
    const std::set<base::FilePath>& child_directories) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!file_path.empty());

  resource_metadata_->RemoveEntryOnUIThread(
      entry.resource_id(),
      base::Bind(&ChangeListProcessor::NotifyForRemoveEntryFromParent,
                 weak_ptr_factory_.GetWeakPtr(),
                 entry.file_info().is_directory(),
                 file_path,
                 child_directories));
}

void ChangeListProcessor::NotifyForRemoveEntryFromParent(
    bool is_directory,
    const base::FilePath& file_path,
    const std::set<base::FilePath>& child_directories,
    FileError error,
    const base::FilePath& parent_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DVLOG(1) << "NotifyForRemoveEntryFromParent " << file_path.value();
  if (error == FILE_ERROR_OK) {
    // Notify parent.
    changed_dirs_.insert(parent_path);

    // Notify children, if any.
    changed_dirs_.insert(child_directories.begin(),
                         child_directories.end());

    // If entry is a directory, notify self.
    if (is_directory)
      changed_dirs_.insert(file_path);
  }

  // Continue.
  ApplyNextEntryProtoAsync();
}

void ChangeListProcessor::RefreshEntry(const ResourceEntry& entry,
                                      const base::FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  resource_metadata_->RefreshEntryOnUIThread(
      entry,
      base::Bind(&ChangeListProcessor::NotifyForRefreshEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 file_path));
}

void ChangeListProcessor::NotifyForRefreshEntry(
    const base::FilePath& old_file_path,
    FileError error,
    const base::FilePath& file_path,
    scoped_ptr<ResourceEntry> entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DVLOG(1) << "NotifyForRefreshEntry " << file_path.value();
  if (error == FILE_ERROR_OK) {
    // Notify old parent.
    changed_dirs_.insert(old_file_path.DirName());

    // Notify new parent.
    changed_dirs_.insert(file_path.DirName());

    // Notify self if entry is a directory.
    if (entry->file_info().is_directory()) {
      // Notify new self.
      changed_dirs_.insert(file_path);
      // Notify old self.
      changed_dirs_.insert(old_file_path);
    }
  }

  ApplyNextEntryProtoAsync();
}

// static
void ChangeListProcessor::FeedToEntryMap(
    ScopedVector<ChangeList> change_lists,
    ResourceEntryMap* entry_map,
    ChangeListToEntryProtoMapUMAStats* uma_stats) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  for (size_t i = 0; i < change_lists.size(); ++i) {
    ChangeList* change_list = change_lists[i];

    std::vector<ResourceEntry>* entries = change_list->mutable_entries();
    for (size_t i = 0; i < entries->size(); ++i) {
      ResourceEntry* entry = &(*entries)[i];
      // Some document entries don't map into files (i.e. sites).
      if (entry->resource_id().empty())
        continue;

      // Count the number of files.
      if (uma_stats) {
        if (!entry->file_info().is_directory()) {
          uma_stats->IncrementNumFiles(
              entry->file_specific_info().is_hosted_document());
        }
        if (entry->shared_with_me())
          uma_stats->IncrementNumSharedWithMeEntries();
      }

      std::pair<ResourceEntryMap::iterator, bool> ret = entry_map->
          insert(std::make_pair(entry->resource_id(), ResourceEntry()));
      if (ret.second)
        ret.first->second.Swap(entry);
      else
        LOG(DFATAL) << "Found duplicate file " << entry->base_name();
    }
  }
}

void ChangeListProcessor::UpdateRootEntry(const base::Closure& closure) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!closure.is_null());

  resource_metadata_->GetResourceEntryByPathOnUIThread(
      util::GetDriveMyDriveRootPath(),
      base::Bind(&ChangeListProcessor::UpdateRootEntryAfterGetEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 closure));
}

void ChangeListProcessor::UpdateRootEntryAfterGetEntry(
    const base::Closure& closure,
    FileError error,
    scoped_ptr<ResourceEntry> root_proto) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!closure.is_null());

  if (error != FILE_ERROR_OK) {
    // TODO(satorux): Need to trigger recovery if root is corrupt.
    LOG(WARNING) << "Failed to get the entry for root directory";
    closure.Run();
    return;
  }
  DCHECK(root_proto.get());

  // The changestamp should always be updated.
  root_proto->mutable_directory_specific_info()->set_changestamp(
      largest_changestamp_);

  resource_metadata_->RefreshEntryOnUIThread(
      *root_proto,
      base::Bind(&ChangeListProcessor::UpdateRootEntryAfterRefreshEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 closure));
}

void ChangeListProcessor::UpdateRootEntryAfterRefreshEntry(
    const base::Closure& closure,
    FileError error,
    const base::FilePath& /* root_path */,
    scoped_ptr<ResourceEntry> /* root_proto */) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!closure.is_null());
  LOG_IF(WARNING, error != FILE_ERROR_OK) << "Failed to refresh root directory";

  closure.Run();
}

void ChangeListProcessor::OnComplete() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  resource_metadata_->SetLargestChangestampOnUIThread(
      largest_changestamp_,
      base::Bind(&RunOnCompleteCallback, on_complete_callback_));
}

void ChangeListProcessor::Clear() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  entry_map_.clear();
  changed_dirs_.clear();
  largest_changestamp_ = 0;
  on_complete_callback_.Reset();
}

}  // namespace drive
