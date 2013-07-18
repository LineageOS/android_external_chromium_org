// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NOTE: No header guards are used, since this file is intended to be expanded
// directly within a block where the SOURCE_TYPE macro is defined.

// Used for global events which don't correspond to a particular entity.
SOURCE_TYPE(NONE)

SOURCE_TYPE(URL_REQUEST)
SOURCE_TYPE(SOCKET_STREAM)
SOURCE_TYPE(PROXY_SCRIPT_DECIDER)
SOURCE_TYPE(CONNECT_JOB)
SOURCE_TYPE(SOCKET)
SOURCE_TYPE(SPDY_SESSION)
SOURCE_TYPE(QUIC_SESSION)
SOURCE_TYPE(HOST_RESOLVER_IMPL_REQUEST)
SOURCE_TYPE(HOST_RESOLVER_IMPL_JOB)
SOURCE_TYPE(DISK_CACHE_ENTRY)
SOURCE_TYPE(MEMORY_CACHE_ENTRY)
SOURCE_TYPE(HTTP_STREAM_JOB)
SOURCE_TYPE(EXPONENTIAL_BACKOFF_THROTTLING)
SOURCE_TYPE(UDP_SOCKET)
SOURCE_TYPE(CERT_VERIFIER_JOB)
SOURCE_TYPE(HTTP_PIPELINED_CONNECTION)
SOURCE_TYPE(DOWNLOAD)
SOURCE_TYPE(FILESTREAM)
SOURCE_TYPE(DNS_PROBER)
SOURCE_TYPE(PROXY_CLIENT_SOCKET)
