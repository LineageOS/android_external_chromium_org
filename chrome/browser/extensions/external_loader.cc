// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/external_loader.h"

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/extensions/external_provider_impl.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace extensions {

ExternalLoader::ExternalLoader()
    : owner_(NULL) {
}

void ExternalLoader::Init(ExternalProviderImpl* owner) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  owner_ = owner;
}

const base::FilePath ExternalLoader::GetBaseCrxFilePath() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // By default, relative paths are not supported.
  // Subclasses that wish to support them should override this method.
  return base::FilePath();
}

void ExternalLoader::OwnerShutdown() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  owner_ = NULL;
}

ExternalLoader::~ExternalLoader() {}

void ExternalLoader::LoadFinished() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (owner_) {
    owner_->SetPrefs(prefs_.release());
  }
}

}  // namespace extensions
