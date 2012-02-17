// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_ERROR_HANDLER_H__
#define CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_ERROR_HANDLER_H__
#pragma once

#include <string>
#include "base/location.h"
#include "chrome/browser/sync/internal_api/includes/unrecoverable_error_handler.h"

namespace browser_sync {

class DataTypeErrorHandler : public UnrecoverableErrorHandler {
 public:
  // Call this to disable a datatype while it is running. This is usually
  // called for a runtime failure that is specific to a datatype.
  virtual void OnSingleDatatypeUnrecoverableError(
      const tracked_objects::Location& from_here,
      const std::string& message) = 0;
 protected:
  virtual ~DataTypeErrorHandler() { }
};

}  // namespace browser_sync
#endif  // CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_ERROR_HANDLER_H__

