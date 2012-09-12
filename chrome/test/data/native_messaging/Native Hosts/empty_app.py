#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This native client will read full messages, but do nothing with them and
# send no responses.

import sys
import struct

while 1:
  # Read the message type (first 4 bytes).
  typeBytes = sys.stdin.read(4)

  if len(typeBytes) == 0:
    break

  # Read the message length (4 bytes).
  textLength = struct.unpack('i', sys.stdin.read(4))[0]

  # Read the text (JSON object) of the message.
  text = sys.stdin.read(textLength)
