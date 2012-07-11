// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_
#define NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "net/base/net_export.h"
#include "net/websockets/websocket_frame.h"

namespace net {

// Parses WebSocket frames from byte stream.
//
// Specification of WebSocket frame format is available at
// <http://tools.ietf.org/html/rfc6455#section-5>.

class NET_EXPORT_PRIVATE WebSocketFrameParser {
 public:
  WebSocketFrameParser();
  ~WebSocketFrameParser();

  // Decodes the given byte stream and stores parsed WebSocket frames in
  // |frames|.
  //
  // If the parser encounters invalid payload length format, Decode() fails
  // and returns false. Once Decode() has failed, the parser refuses to decode
  // any more data and future invocations of Decode() will simply return false.
  //
  // Payload data of parsed WebSocket frames may be incomplete; see comments in
  // websocket_frame.h for more details.
  bool Decode(const char* data,
              size_t length,
              ScopedVector<WebSocketFrameChunk>* frame_chunks);

  // Returns true if the parser has ever failed to decode a WebSocket frame.
  // TODO(yutak): Provide human-readable description of failure.
  bool failed() const { return failed_; }

 private:
  // Tries to decode a frame header from |current_read_pos_|.
  // If successful, this function updates |current_read_pos_|,
  // |current_frame_header_|, and |masking_key_| (if available).
  // This function may set |failed_| to true if it observes a corrupt frame.
  // If there is not enough data in the remaining buffer to parse a frame
  // header, this function returns without doing anything.
  void DecodeFrameHeader();

  // Decodes frame payload and creates a WebSocketFrameChunk object.
  // This function updates |current_read_pos_| and |frame_offset_| after
  // parsing. This function returns a frame object even if no payload data is
  // available at this moment, so the receiver could make use of frame header
  // information. If the end of frame is reached, this function clears
  // |current_frame_header_|, |frame_offset_| and |masking_key_|.
  scoped_ptr<WebSocketFrameChunk> DecodeFramePayload(bool first_chunk);

  // Internal buffer to store the data to parse.
  std::vector<char> buffer_;

  // Position in |buffer_| where the next round of parsing starts.
  size_t current_read_pos_;

  // Frame header and masking key of the current frame.
  // |masking_key_| is filled with zeros if the current frame is not masked.
  scoped_ptr<WebSocketFrameHeader> current_frame_header_;
  char masking_key_[WebSocketFrameHeader::kMaskingKeyLength];

  // Amount of payload data read so far for the current frame.
  uint64 frame_offset_;

  bool failed_;

  DISALLOW_COPY_AND_ASSIGN(WebSocketFrameParser);
};

}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_
