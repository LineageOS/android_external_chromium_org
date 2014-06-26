/*
 * Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Error codes and data structures used to report errors when loading a nexe.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PLUGIN_ERROR_H
#define NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PLUGIN_ERROR_H

#include <string>

#include "native_client/src/include/nacl_macros.h"
#include "ppapi/c/private/ppb_nacl_private.h"

namespace plugin {

class ErrorInfo {
 public:
  ErrorInfo() {
    SetReport(PP_NACL_ERROR_UNKNOWN, std::string());
  }

  void SetReport(PP_NaClError error_code, const std::string& message) {
    error_code_ = error_code;
    message_ = message;
  }

  PP_NaClError error_code() const {
    return error_code_;
  }

  const std::string& message() const {
    return message_;
  }

 private:
  PP_NaClError error_code_;
  std::string message_;
  NACL_DISALLOW_COPY_AND_ASSIGN(ErrorInfo);
};

}  // namespace plugin

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PLUGIN_ERROR_H
