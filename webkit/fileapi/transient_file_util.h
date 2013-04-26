// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_TRANSIENT_FILE_UTIL_H_
#define WEBKIT_FILEAPI_TRANSIENT_FILE_UTIL_H_

#include "base/memory/scoped_ptr.h"
#include "webkit/fileapi/isolated_file_util.h"
#include "webkit/storage/webkit_storage_export.h"

namespace fileapi {

class FileSystemOperationContext;

class WEBKIT_STORAGE_EXPORT_PRIVATE TransientFileUtil
    : public IsolatedFileUtil {
 public:
  TransientFileUtil() {}
  virtual ~TransientFileUtil() {}

  // IsolatedFileUtil overrides.
  virtual webkit_blob::ScopedFile CreateSnapshotFile(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      base::PlatformFileError* error,
      base::PlatformFileInfo* file_info,
      base::FilePath* platform_path) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(TransientFileUtil);
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_TRANSIENT_FILE_UTIL_H_
