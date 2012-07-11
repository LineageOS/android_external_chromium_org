// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_PLATFORM_FONT_PANGO_H_
#define UI_GFX_PLATFORM_FONT_PANGO_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/platform_font.h"

class SkTypeface;
class SkPaint;

namespace gfx {

class UI_EXPORT PlatformFontPango : public PlatformFont {
 public:
  PlatformFontPango();
  explicit PlatformFontPango(NativeFont native_font);
  PlatformFontPango(const std::string& font_name, int font_size);

  // Converts |gfx_font| to a new pango font. Free the returned font with
  // pango_font_description_free().
  static PangoFontDescription* PangoFontFromGfxFont(const gfx::Font& gfx_font);

  // Resets and reloads the cached system font used by the default constructor.
  // This function is useful when the system font has changed, for example, when
  // the locale has changed.
  static void ReloadDefaultFont();

  // Position as an offset from the height of the drawn text, used to draw
  // an underline. This is a negative number, so the underline would be
  // drawn at y + height + underline_position;
  double underline_position() const;
  // The thickness to draw the underline.
  double underline_thickness() const;

  // Overridden from PlatformFont:
  virtual Font DeriveFont(int size_delta, int style) const OVERRIDE;
  virtual int GetHeight() const OVERRIDE;
  virtual int GetBaseline() const OVERRIDE;
  virtual int GetAverageCharacterWidth() const OVERRIDE;
  virtual int GetStringWidth(const string16& text) const OVERRIDE;
  virtual int GetExpectedTextWidth(int length) const OVERRIDE;
  virtual int GetStyle() const OVERRIDE;
  virtual std::string GetFontName() const OVERRIDE;
  virtual int GetFontSize() const OVERRIDE;
  virtual NativeFont GetNativeFont() const OVERRIDE;

 private:
  // Create a new instance of this object with the specified properties. Called
  // from DeriveFont.
  PlatformFontPango(SkTypeface* typeface,
                    const std::string& name,
                    int size,
                    int style);
  virtual ~PlatformFontPango();

  // Initialize this object.
  void InitWithNameAndSize(const std::string& font_name, int font_size);
  void InitWithTypefaceNameSizeAndStyle(SkTypeface* typeface,
                                        const std::string& name,
                                        int size,
                                        int style);
  void InitFromPlatformFont(const PlatformFontPango* other);

  // Potentially slow call to get pango metrics (average width, underline info).
  void InitPangoMetrics();

  // Setup a Skia context to use the current typeface
  void PaintSetup(SkPaint* paint) const;

  // Make |this| a copy of |other|.
  void CopyFont(const Font& other);

  // The average width of a character, initialized and cached if needed.
  double GetAverageWidth() const;

  // These two both point to the same SkTypeface. We use the SkAutoUnref to
  // handle the reference counting, but without @typeface_ we would have to
  // cast the SkRefCnt from @typeface_helper_ every time.
  scoped_ptr<SkAutoUnref> typeface_helper_;
  SkTypeface* typeface_;

  // Additional information about the face
  // Skia actually expects a family name and not a font name.
  std::string font_family_;
  int font_size_pixels_;
  int style_;

  // Cached metrics, generated at construction
  int height_pixels_;
  int ascent_pixels_;

  // The pango metrics are much more expensive so we wait until we need them
  // to compute them.
  bool pango_metrics_inited_;
  double average_width_pixels_;
  double underline_position_pixels_;
  double underline_thickness_pixels_;

  // The default font, used for the default constructor.
  static Font* default_font_;
};

}  // namespace gfx

#endif  // UI_GFX_PLATFORM_FONT_PANGO_H_
