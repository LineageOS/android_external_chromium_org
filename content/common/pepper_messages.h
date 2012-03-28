// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included message file, no traditional include guard
#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/proxy/ppapi_param_traits.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START PepperMsgStart

// Pepper (non-file-system) messages sent from the renderer to the browser.

IPC_SYNC_MESSAGE_CONTROL1_1(PepperMsg_GetLocalTimeZoneOffset,
                            base::Time /* t */,
                            double /* result */)
