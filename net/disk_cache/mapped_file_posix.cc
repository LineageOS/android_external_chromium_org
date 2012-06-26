// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/mapped_file.h"

#include <errno.h>
#include <sys/mman.h>

#include "base/file_path.h"
#include "base/logging.h"
#include "net/disk_cache/disk_cache.h"

namespace disk_cache {

void* MappedFile::Init(const FilePath& name, size_t size) {
  DCHECK(!init_);
  if (init_ || !File::Init(name))
    return NULL;

  size_t temp_len = size ? size : 4096;
  if (!size)
    size = GetLength();

  buffer_ = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                 platform_file(), 0);
  init_ = true;
  view_size_ = size;
  DCHECK(reinterpret_cast<intptr_t>(buffer_) != -1);
  if (reinterpret_cast<intptr_t>(buffer_) == -1)
    buffer_ = 0;

  // Make sure we detect hardware failures reading the headers.
  scoped_array<char> temp(new char[temp_len]);
  if (!Read(temp.get(), temp_len, 0))
    return NULL;

  return buffer_;
}

bool MappedFile::Load(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Read(block->buffer(), block->size(), offset);
}

bool MappedFile::Store(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Write(block->buffer(), block->size(), offset);
}

void MappedFile::Flush() {
}

MappedFile::~MappedFile() {
  if (!init_)
    return;

  if (buffer_) {
    int ret = munmap(buffer_, view_size_);
    DCHECK_EQ(0, ret);
  }
}

}  // namespace disk_cache
