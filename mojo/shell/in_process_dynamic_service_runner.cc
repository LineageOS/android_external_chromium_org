// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/shell/in_process_dynamic_service_runner.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_proxy.h"
#include "mojo/public/platform/native/system_thunks.h"

namespace mojo {
namespace shell {

InProcessDynamicServiceRunner::InProcessDynamicServiceRunner(
    Context* context)
    : keep_alive_(context) {
}

InProcessDynamicServiceRunner::~InProcessDynamicServiceRunner() {
  if (thread_) {
    DCHECK(thread_->HasBeenStarted());
    DCHECK(!thread_->HasBeenJoined());
    thread_->Join();
  }

  // It is important to let the thread exit before unloading the DSO because
  // the library may have registered thread-local data and destructors to run
  // on thread termination.
  app_library_.Reset(base::NativeLibrary());
}

void InProcessDynamicServiceRunner::Start(
    const base::FilePath& app_path,
    ScopedMessagePipeHandle service_handle,
    const base::Closure& app_completed_callback) {
  app_path_ = app_path;

  DCHECK(!service_handle_.is_valid());
  service_handle_ = service_handle.Pass();

  DCHECK(app_completed_callback_runner_.is_null());
  app_completed_callback_runner_ = base::Bind(&base::TaskRunner::PostTask,
                                              base::MessageLoopProxy::current(),
                                              FROM_HERE,
                                              app_completed_callback);

  DCHECK(!thread_);
  thread_.reset(new base::DelegateSimpleThread(this, "app_thread"));
  thread_->Start();
}

void InProcessDynamicServiceRunner::Run() {
  DVLOG(2) << "Loading/running Mojo app in process from library: "
           << app_path_.value();

  do {
    base::NativeLibraryLoadError error;
    app_library_.Reset(base::LoadNativeLibrary(app_path_, &error));
    if (!app_library_.is_valid()) {
      LOG(ERROR) << "Failed to load app library (error: " << error.ToString()
                 << ")";
      break;
    }

    MojoSetSystemThunksFn mojo_set_system_thunks_fn =
        reinterpret_cast<MojoSetSystemThunksFn>(app_library_.GetFunctionPointer(
            "MojoSetSystemThunks"));
    if (mojo_set_system_thunks_fn) {
      MojoSystemThunks system_thunks = MojoMakeSystemThunks();
      size_t expected_size = mojo_set_system_thunks_fn(&system_thunks);
      if (expected_size > sizeof(MojoSystemThunks)) {
        LOG(ERROR)
            << "Invalid app library: expected MojoSystemThunks size: "
            << expected_size;
        break;
      }
    } else {
      // In the component build, Mojo Apps link against mojo_system_impl.
#if !defined(COMPONENT_BUILD)
      // Strictly speaking this is not required, but it's very unusual to have
      // an app that doesn't require the basic system library.
      LOG(WARNING) << "MojoSetSystemThunks not found in app library";
#endif
    }

    typedef MojoResult (*MojoMainFunction)(MojoHandle);
    MojoMainFunction main_function = reinterpret_cast<MojoMainFunction>(
        app_library_.GetFunctionPointer("MojoMain"));
    if (!main_function) {
      LOG(ERROR) << "Entrypoint MojoMain not found";
      break;
    }

    // |MojoMain()| takes ownership of the service handle.
    MojoResult result = main_function(service_handle_.release().value());
    if (result < MOJO_RESULT_OK)
      LOG(ERROR) << "MojoMain returned an error: " << result;
  } while (false);

  bool success = app_completed_callback_runner_.Run();
  app_completed_callback_runner_.Reset();
  LOG_IF(ERROR, !success) << "Failed post run app_completed_callback";
}

}  // namespace shell
}  // namespace mojo
