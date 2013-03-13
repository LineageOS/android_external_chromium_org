// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PICTURE_PILE_IMPL_H_
#define CC_PICTURE_PILE_IMPL_H_

#include <list>
#include <map>

#include "cc/cc_export.h"
#include "cc/picture_pile_base.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace cc {
struct RenderingStats;

class CC_EXPORT PicturePileImpl : public PicturePileBase {
 public:
  static scoped_refptr<PicturePileImpl> Create();

  // Get paint-safe version of this picture for a specific thread.
  PicturePileImpl* GetCloneForDrawingOnThread(unsigned thread_index) const;

  // Make paint-safe versions of this picture pile.
  void CloneForDrawing(int num_threads);

  // Raster a subrect of this PicturePileImpl into the given canvas.
  // It's only safe to call paint on a cloned version.
  // It is assumed that contentsScale has already been applied to this canvas.
  void Raster(
      SkCanvas* canvas,
      gfx::Rect canvas_rect,
      float contents_scale,
      int64* total_pixels_rasterized);

  void GatherPixelRefs(
      gfx::Rect content_rect,
      float contents_scale,
      std::list<skia::LazyPixelRef*>& pixel_refs);

  void PushPropertiesTo(PicturePileImpl* other);

  skia::RefPtr<SkPicture> GetFlattenedPicture();

  struct Analysis {
    Analysis();

    bool is_solid_color;
    bool is_transparent;
    bool is_cheap_to_raster;
    SkColor solid_color;
  };

  void AnalyzeInRect(const gfx::Rect& content_rect,
                     float contents_scale,
                     Analysis* analysis);

 protected:
  friend class PicturePile;

  PicturePileImpl();
  virtual ~PicturePileImpl();

  typedef std::vector<scoped_refptr<PicturePileImpl> > PicturePileVector;
  PicturePileVector clones_;

  DISALLOW_COPY_AND_ASSIGN(PicturePileImpl);
};

}  // namespace cc

#endif  // CC_PICTURE_PILE_IMPL_H_
