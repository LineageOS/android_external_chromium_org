// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/env.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/root_window_host.h"
#include "ui/aura/window.h"

namespace aura {

// static
Env* Env::instance_ = NULL;

////////////////////////////////////////////////////////////////////////////////
// Env, public:

Env::Env() {
#if !defined(OS_MACOSX)
  dispatcher_.reset(CreateDispatcher());
#endif
}

Env::~Env() {}

// static
Env* Env::GetInstance() {
  if (!instance_)
    instance_ = new Env;
  return instance_;
}

// static
void Env::DeleteInstance() {
  delete instance_;
  instance_ = NULL;
}

void Env::AddObserver(EnvObserver* observer) {
  observers_.AddObserver(observer);
}

void Env::RemoveObserver(EnvObserver* observer) {
  observers_.RemoveObserver(observer);
}

#if !defined(OS_MACOSX)
MessageLoop::Dispatcher* Env::GetDispatcher() {
  return dispatcher_.get();
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Env, private:

void Env::NotifyWindowInitialized(Window* window) {
  FOR_EACH_OBSERVER(EnvObserver, observers_, OnWindowInitialized(window));
}

}  // namespace aura
