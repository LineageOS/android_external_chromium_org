// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/message_filter.h"

#include "base/memory/ref_counted.h"
#include "ipc/ipc_channel.h"

namespace IPC {

MessageFilter::MessageFilter() {}

void MessageFilter::OnFilterAdded(Sender* sender) {}

void MessageFilter::OnFilterRemoved() {}

void MessageFilter::OnChannelConnected(int32 peer_pid) {}

void MessageFilter::OnChannelError() {}

void MessageFilter::OnChannelClosing() {}

bool MessageFilter::OnMessageReceived(const Message& message) {
  return false;
}

bool MessageFilter::GetSupportedMessageClasses(
    std::vector<uint32>* /*supported_message_classes*/) const {
  return false;
}

MessageFilter::~MessageFilter() {}

}  // namespace IPC
