// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/public/write_transaction.h"

#include "sync/syncable/write_transaction.h"

namespace csync {

//////////////////////////////////////////////////////////////////////////
// WriteTransaction member definitions
WriteTransaction::WriteTransaction(const tracked_objects::Location& from_here,
                                   UserShare* share)
    : BaseTransaction(share),
      transaction_(NULL) {
  transaction_ = new syncable::WriteTransaction(from_here, syncable::SYNCAPI,
                                                share->directory.get());
}

WriteTransaction::~WriteTransaction() {
  delete transaction_;
}

syncable::BaseTransaction* WriteTransaction::GetWrappedTrans() const {
  return transaction_;
}

} // namespace csync
