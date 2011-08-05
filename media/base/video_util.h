// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_VIDEO_UTIL_H_
#define MEDIA_BASE_VIDEO_UTIL_H_

#include "base/basictypes.h"

namespace media {

class VideoFrame;

// Copies a plane of YUV source into a VideoFrame object, taking into account
// source and destinations dimensions.
//
// NOTE: rows is *not* the same as height!
void CopyYPlane(const uint8* source, int stride, int rows, VideoFrame* frame);
void CopyUPlane(const uint8* source, int stride, int rows, VideoFrame* frame);
void CopyVPlane(const uint8* source, int stride, int rows, VideoFrame* frame);

}  // namespace media

#endif  // MEDIA_BASE_VIDEO_UTIL_H_
