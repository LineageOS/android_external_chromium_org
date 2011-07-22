// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_FILE_DATA_SOURCE_FACTORY_H_
#define MEDIA_FILTERS_FILE_DATA_SOURCE_FACTORY_H_

#include "base/compiler_specific.h"
#include "media/base/filter_factories.h"

namespace media {

class FileDataSourceFactory : public DataSourceFactory {
 public:
  FileDataSourceFactory();
  virtual ~FileDataSourceFactory();

  // DataSourceFactory methods.
  virtual void Build(const std::string& url, const BuildCB& callback) OVERRIDE;
  virtual DataSourceFactory* Clone() const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileDataSourceFactory);
};

}  // namespace media

#endif  // MEDIA_FILTERS_FILE_DATA_SOURCE_FACTORY_H_
