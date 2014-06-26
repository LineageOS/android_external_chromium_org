// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/ev_bubble_decoration.h"

#import "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/location_bar/location_icon_decoration.h"
#include "grit/theme_resources.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_elider.h"

namespace {

// TODO(shess): In general, decorations that don't fit in the
// available space are omitted.  This one never goes to omitted, it
// sticks at 150px, which AFAICT follows the Windows code.  Since the
// Layout() code doesn't take this into account, it's possible the
// control could end up with display artifacts, though things still
// work (and don't crash).
// http://crbug.com/49822

// Minimum acceptable width for the ev bubble.
const CGFloat kMinElidedBubbleWidth = 150.0;

// Maximum amount of available space to make the bubble, subject to
// |kMinElidedBubbleWidth|.
const float kMaxBubbleFraction = 0.5;

// The info-bubble point should look like it points to the bottom of the lock
// icon. Determined with Pixie.app.
const CGFloat kPageInfoBubblePointYOffset = 6.0;

// TODO(shess): This is ugly, find a better way.  Using it right now
// so that I can crib from gtk and still be able to see that I'm using
// the same values easily.
NSColor* ColorWithRGBBytes(int rr, int gg, int bb) {
  DCHECK_LE(rr, 255);
  DCHECK_LE(bb, 255);
  DCHECK_LE(gg, 255);
  return [NSColor colorWithCalibratedRed:static_cast<float>(rr)/255.0
                                   green:static_cast<float>(gg)/255.0
                                    blue:static_cast<float>(bb)/255.0
                                   alpha:1.0];
}

}  // namespace

EVBubbleDecoration::EVBubbleDecoration(LocationIconDecoration* location_icon)
    : location_icon_(location_icon) {
  // Color tuples stolen from location_bar_view_gtk.cc.
  SetTextColor(ColorWithRGBBytes(0x07, 0x95, 0x00));
}

EVBubbleDecoration::~EVBubbleDecoration() {}

void EVBubbleDecoration::SetFullLabel(NSString* label) {
  full_label_.reset([label retain]);
  SetLabel(full_label_);
}

NSPoint EVBubbleDecoration::GetBubblePointInFrame(NSRect frame) {
  NSRect image_rect = GetImageRectInFrame(frame);
  return NSMakePoint(NSMidX(image_rect),
                     NSMaxY(image_rect) - kPageInfoBubblePointYOffset);
}

CGFloat EVBubbleDecoration::GetWidthForSpace(CGFloat width) {
  // Limit with to not take up too much of the available width, but
  // also don't let it shrink too much.
  width = std::max(width * kMaxBubbleFraction, kMinElidedBubbleWidth);

  // Use the full label if it fits.
  NSImage* image = GetImage();
  const CGFloat all_width = GetWidthForImageAndLabel(image, full_label_);
  if (all_width <= width) {
    SetLabel(full_label_);
    return all_width;
  }

  // Width left for laying out the label.
  const CGFloat width_left = width - GetWidthForImageAndLabel(image, @"");

  // Middle-elide the label to fit |width_left|.  This leaves the
  // prefix and the trailing country code in place.
  NSString* elided_label = base::SysUTF16ToNSString(
      gfx::ElideText(base::SysNSStringToUTF16(full_label_),
                     gfx::FontList(gfx::Font(GetFont())),
                     width_left, gfx::ELIDE_MIDDLE));

  // Use the elided label.
  SetLabel(elided_label);
  return GetWidthForImageAndLabel(image, elided_label);
}

// Pass mouse operations through to location icon.
bool EVBubbleDecoration::IsDraggable() {
  return location_icon_->IsDraggable();
}

NSPasteboard* EVBubbleDecoration::GetDragPasteboard() {
  return location_icon_->GetDragPasteboard();
}

NSImage* EVBubbleDecoration::GetDragImage() {
  return location_icon_->GetDragImage();
}

NSRect EVBubbleDecoration::GetDragImageFrame(NSRect frame) {
  return GetImageRectInFrame(frame);
}

bool EVBubbleDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  return location_icon_->OnMousePressed(frame, location);
}

bool EVBubbleDecoration::AcceptsMousePress() {
  return true;
}

ui::NinePartImageIds EVBubbleDecoration::GetBubbleImageIds() {
  return IMAGE_GRID(IDR_OMNIBOX_EV_BUBBLE);
}
