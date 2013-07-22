// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_server_properties.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace net {

const char kAlternateProtocolHeader[] = "Alternate-Protocol";
// The order of these strings much match the order of the enum definition
// for AlternateProtocol.
const char* const kAlternateProtocolStrings[] = {
  "npn-spdy/1",
  "npn-spdy/2",
  "npn-spdy/3",
  "npn-spdy/3.1",
  "npn-spdy/4a2",
  "quic"
};
const char kBrokenAlternateProtocol[] = "Broken";

const char* AlternateProtocolToString(AlternateProtocol protocol) {
  switch (protocol) {
    case NPN_SPDY_1:
    case NPN_SPDY_2:
    case NPN_SPDY_3:
    case NPN_SPDY_3_1:
    case NPN_SPDY_4A2:
    case QUIC:
      DCHECK_LT(static_cast<size_t>(protocol),
                arraysize(kAlternateProtocolStrings));
      return kAlternateProtocolStrings[protocol];
    case ALTERNATE_PROTOCOL_BROKEN:
      return kBrokenAlternateProtocol;
    case UNINITIALIZED_ALTERNATE_PROTOCOL:
      return "Uninitialized";
    default:
      NOTREACHED();
      return "";
  }
}

AlternateProtocol AlternateProtocolFromString(const std::string& protocol) {
  for (int i = NPN_SPDY_1; i < NUM_ALTERNATE_PROTOCOLS; ++i)
    if (protocol == kAlternateProtocolStrings[i])
      return static_cast<AlternateProtocol>(i);
  if (protocol == kBrokenAlternateProtocol)
    return ALTERNATE_PROTOCOL_BROKEN;
  return UNINITIALIZED_ALTERNATE_PROTOCOL;
}

AlternateProtocol AlternateProtocolFromNextProto(NextProto next_proto) {
  switch (next_proto) {
    case kProtoSPDY2:
      return NPN_SPDY_2;
    case kProtoSPDY3:
      return NPN_SPDY_3;
    case kProtoSPDY31:
      return NPN_SPDY_3_1;
    case kProtoSPDY4a2:
      return NPN_SPDY_4A2;
    case kProtoQUIC1SPDY3:
      return QUIC;

    case kProtoUnknown:
    case kProtoHTTP11:
    case kProtoSPDY1:
    case kProtoSPDY21:
      break;
  }

  NOTREACHED() << "Invalid NextProto: " << next_proto;
  return UNINITIALIZED_ALTERNATE_PROTOCOL;
}

std::string PortAlternateProtocolPair::ToString() const {
  return base::StringPrintf("%d:%s", port,
                            AlternateProtocolToString(protocol));
}

}  // namespace net
