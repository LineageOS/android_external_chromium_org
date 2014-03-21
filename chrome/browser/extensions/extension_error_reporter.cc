// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_error_reporter.h"

#include "build/build_config.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/simple_message_box.h"

ExtensionErrorReporter* ExtensionErrorReporter::instance_ = NULL;

// static
void ExtensionErrorReporter::Init(bool enable_noisy_errors) {
  if (!instance_) {
    instance_ = new ExtensionErrorReporter(enable_noisy_errors);
  }
}

// static
ExtensionErrorReporter* ExtensionErrorReporter::GetInstance() {
  CHECK(instance_) << "Init() was never called";
  return instance_;
}

ExtensionErrorReporter::ExtensionErrorReporter(bool enable_noisy_errors)
    : ui_loop_(base::MessageLoop::current()),
      enable_noisy_errors_(enable_noisy_errors) {
}

ExtensionErrorReporter::~ExtensionErrorReporter() {}

void ExtensionErrorReporter::ReportError(const base::string16& message,
                                         bool be_noisy,
                                         bool* user_response) {
  // NOTE: There won't be a ui_loop_ in the unit test environment.
  if (ui_loop_) {
    CHECK(base::MessageLoop::current() == ui_loop_)
        << "ReportError can only be called from the UI thread.";
  }

  errors_.push_back(message);

  // TODO(aa): Print the error message out somewhere better. I think we are
  // going to need some sort of 'extension inspector'.
  LOG(WARNING) << "Extension error: " << message;

  if (enable_noisy_errors_ && be_noisy) {
    if (user_response) {
      *user_response =
          chrome::MESSAGE_BOX_RESULT_YES ==
          chrome::ShowMessageBox(NULL,
                                 base::ASCIIToUTF16("Extension error"),
                                 message,
                                 chrome::MESSAGE_BOX_TYPE_QUESTION);
    } else {
      chrome::ShowMessageBox(NULL,
                             base::ASCIIToUTF16("Extension error"),
                             message,
                             chrome::MESSAGE_BOX_TYPE_WARNING);
    }
  }
}

const std::vector<base::string16>* ExtensionErrorReporter::GetErrors() {
  return &errors_;
}

void ExtensionErrorReporter::ClearErrors() {
  errors_.clear();
}
