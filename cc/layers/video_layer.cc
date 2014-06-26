// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/video_layer.h"

#include "cc/layers/video_layer_impl.h"

namespace cc {

scoped_refptr<VideoLayer> VideoLayer::Create(VideoFrameProvider* provider) {
  return make_scoped_refptr(new VideoLayer(provider));
}

VideoLayer::VideoLayer(VideoFrameProvider* provider) : provider_(provider) {
  DCHECK(provider_);
}

VideoLayer::~VideoLayer() {}

scoped_ptr<LayerImpl> VideoLayer::CreateLayerImpl(LayerTreeImpl* tree_impl) {
  return VideoLayerImpl::Create(tree_impl, id(), provider_).PassAs<LayerImpl>();
}

bool VideoLayer::Update(ResourceUpdateQueue* queue,
                        const OcclusionTracker<Layer>* occlusion) {
  bool updated = Layer::Update(queue, occlusion);

  // Video layer doesn't update any resources from the main thread side,
  // but repaint rects need to be sent to the VideoLayerImpl via commit.
  //
  // This is the inefficient legacy redraw path for videos.  It's better to
  // communicate this directly to the VideoLayerImpl.
  updated |= !update_rect_.IsEmpty();

  return updated;
}

}  // namespace cc
