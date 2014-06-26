// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/fileapi/readahead_file_stream_reader.h"

#include <algorithm>

#include "base/message_loop/message_loop.h"
#include "base/numerics/safe_conversions.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

using webkit_blob::FileStreamReader;

namespace {

const size_t kDesiredNumberOfBuffers = 2;  // So we are always one buffer ahead.
const int kBufferSize = 1024*1024;  // 1MB to minimize transaction costs.

}  // namespace

ReadaheadFileStreamReader::ReadaheadFileStreamReader(FileStreamReader* source)
    : source_(source),
      source_error_(0),
      source_has_pending_read_(false),
      weak_factory_(this) {
}

ReadaheadFileStreamReader::~ReadaheadFileStreamReader() {}

int ReadaheadFileStreamReader::Read(
    net::IOBuffer* buf, int buf_len, const net::CompletionCallback& callback) {
  DCHECK(!pending_sink_buffer_.get());
  DCHECK(pending_read_callback_.is_null());

  ReadFromSourceIfNeeded();

  scoped_refptr<net::DrainableIOBuffer> sink =
      new net::DrainableIOBuffer(buf, buf_len);
  int result = FinishReadFromCacheOrStoredError(sink);

  // We are waiting for an source read to complete, so save the request.
  if (result == net::ERR_IO_PENDING) {
    DCHECK(!pending_sink_buffer_.get());
    DCHECK(pending_read_callback_.is_null());
    pending_sink_buffer_ = sink;
    pending_read_callback_ = callback;
  }

  return result;
}

int64 ReadaheadFileStreamReader::GetLength(
    const net::Int64CompletionCallback& callback) {
  return source_->GetLength(callback);
}

int ReadaheadFileStreamReader::FinishReadFromCacheOrStoredError(
    net::DrainableIOBuffer* sink) {
  // If we don't have any ready cache, return the pending read code, or
  // the stored error code.
  if (buffers_.empty()) {
    if (source_.get()) {
      DCHECK(source_has_pending_read_);
      return net::ERR_IO_PENDING;
    } else {
      return source_error_;
    }
  }

  while (sink->BytesRemaining() > 0 && !buffers_.empty()) {
    net::DrainableIOBuffer* source_buffer = buffers_.front().get();

    DCHECK(source_buffer->BytesRemaining() > 0);

    int copy_len = std::min(source_buffer->BytesRemaining(),
                            sink->BytesRemaining());
    std::copy(source_buffer->data(), source_buffer->data() + copy_len,
              sink->data());

    source_buffer->DidConsume(copy_len);
    sink->DidConsume(copy_len);

    if (source_buffer->BytesRemaining() == 0) {
      buffers_.pop();

      // Get a new buffer to replace the one we just used up.
      ReadFromSourceIfNeeded();
    }
  }

  return sink->BytesConsumed();
}

void ReadaheadFileStreamReader::ReadFromSourceIfNeeded() {
  if (!source_.get() || source_has_pending_read_ ||
      buffers_.size() >= kDesiredNumberOfBuffers) {
    return;
  }

  source_has_pending_read_ = true;

  scoped_refptr<net::IOBuffer> buf(new net::IOBuffer(kBufferSize));
  int result = source_->Read(
      buf,
      kBufferSize,
      base::Bind(&ReadaheadFileStreamReader::OnFinishReadFromSource,
                 weak_factory_.GetWeakPtr(), buf));

  if (result != net::ERR_IO_PENDING)
    OnFinishReadFromSource(buf, result);
}

void ReadaheadFileStreamReader::OnFinishReadFromSource(net::IOBuffer* buf,
                                                       int result) {
  DCHECK(result != net::ERR_IO_PENDING);
  DCHECK(source_has_pending_read_);
  source_has_pending_read_ = false;

  // Either store the data read from |source_|, or store the error code.
  if (result > 0) {
    scoped_refptr<net::DrainableIOBuffer> drainable_buffer(
        new net::DrainableIOBuffer(buf, result));
    buffers_.push(drainable_buffer);
    ReadFromSourceIfNeeded();
  } else {
    source_.reset();
    source_error_ = result;
  }

  // If there's a read request waiting for the source FileStreamReader to
  // finish reading, fulfill that request now from the cache or stored error.
  if (pending_sink_buffer_.get()) {
    DCHECK(!pending_read_callback_.is_null());

    // Free the pending callback before running it, as the callback often
    // dispatches another read.
    scoped_refptr<net::DrainableIOBuffer> sink = pending_sink_buffer_;
    pending_sink_buffer_ = NULL;
    net::CompletionCallback completion_callback = pending_read_callback_;
    pending_read_callback_.Reset();

    completion_callback.Run(FinishReadFromCacheOrStoredError(sink));
  }
}
