// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/tab_cell.h"
#import "third_party/GTM/AppKit/GTMTheme.h"
#import "third_party/GTM/AppKit/GTMNSColor+Luminance.h"

#define INSET_MULTIPLIER 2.0/3.0
#define CP1_MULTIPLIER 1.0/3.0
#define CP2_MULTIPLIER 3.0/8.0

namespace {
  NSGradient *getGradientForStyle(bool selected, bool active) {
    // Workaround for
    //  rdar://6857649 - NSGradient fails when using LAB Colorspace
    // Once that's fixed we can remove this function and just call through
    // to -[GTMTheme gradientForStyle:active:] directly with
    // GTMThemeStyleTabBarSelected/GTMThemeStyleTabBarDeselected
    NSUInteger startHighlight, endHighlight;
    if (selected) {
      startHighlight = GTMColorationLightHighlight;
      endHighlight = GTMColorationLightMidtone;
    } else {
      startHighlight = GTMColorationBaseShadow;
      endHighlight = GTMColorationBaseShadow;
    }

    NSColor *backgroundColor = [[GTMTheme defaultTheme] backgroundColor];
    NSColor *startColor = [backgroundColor gtm_colorAdjustedFor:startHighlight
                                                          faded:!active];
    NSColor *endColor = [backgroundColor gtm_colorAdjustedFor:endHighlight
                                                        faded:!active];
    NSColorSpace *genericRGB = [NSColorSpace genericRGBColorSpace];
    startColor = [startColor colorUsingColorSpace:genericRGB];
    endColor = [endColor colorUsingColorSpace:genericRGB];

    return [[[NSGradient alloc] initWithStartingColor:startColor
                                          endingColor:endColor]
               autorelease];

  }
}  // namespace

@implementation TabCell

- (id)initTextCell:(NSString *)aString {
  self = [super initTextCell:aString];
  if (self != nil) {
    // nothing for now...
  }
  return self;
}

- (NSBackgroundStyle)interiorBackgroundStyle {
  return [[GTMTheme defaultTheme]
              interiorBackgroundStyleForStyle:GTMThemeStyleTabBarSelected
                                       active:YES];
}

// Override drawing the button so that it looks like a Chromium tab instead
// of just a normal MacOS button.
- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView {
  [[NSGraphicsContext currentContext] saveGraphicsState];
  NSRect rect = cellFrame;
  BOOL active =
      [[controlView window] isKeyWindow] || [[controlView window] isMainWindow];

  BOOL selected = [(NSButton *)controlView state];

  // Inset by 0.5 in order to draw on pixels rather than on borders (which would
  // cause blurry pixels). Decrease height by 1 in order to move away from the
  // edge for the dark shadow.
  rect = NSInsetRect(rect, 0.5, selected ? 0 : -0.5);
  rect.size.height -= 1;
  rect.origin.y += 1;

  NSPoint bottomLeft = NSMakePoint(NSMinX(rect), NSMaxY(rect));
  NSPoint bottomRight = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
  NSPoint topRight =
      NSMakePoint(NSMaxX(rect) - INSET_MULTIPLIER * NSHeight(rect),
                  NSMinY(rect));
  NSPoint topLeft =
      NSMakePoint(NSMinX(rect)  + INSET_MULTIPLIER * NSHeight(rect),
                  NSMinY(rect));

  float baseControlPointOutset = NSHeight(rect) * CP1_MULTIPLIER;
  float bottomControlPointInset = NSHeight(rect) * CP2_MULTIPLIER;

  // Outset many of these values by 1 to cause the fill to bleed outside the
  // clip area
  NSBezierPath *path = [NSBezierPath bezierPath];
  [path moveToPoint:NSMakePoint(bottomLeft.x - 1, bottomLeft.y + 1)];
  [path lineToPoint:NSMakePoint(bottomLeft.x - 1, bottomLeft.y)];
  [path lineToPoint:bottomLeft];
  [path curveToPoint:topLeft
       controlPoint1:NSMakePoint(bottomLeft.x + baseControlPointOutset,
                                 bottomLeft.y)
       controlPoint2:NSMakePoint(topLeft.x - bottomControlPointInset,
                                 topLeft.y)];
  [path lineToPoint:topRight];
  [path curveToPoint:bottomRight
       controlPoint1:NSMakePoint(topRight.x + bottomControlPointInset,
                                 topRight.y)
       controlPoint2:NSMakePoint(bottomRight.x - baseControlPointOutset,
                                 bottomRight.y)];
  [path lineToPoint:NSMakePoint(bottomRight.x + 1, bottomRight.y)];
  [path lineToPoint:NSMakePoint(bottomRight.x + 1, bottomRight.y + 1)];

  NSGradient *gradient = nil;

  if (selected) {
    gradient = getGradientForStyle(true, active);
    // Stroke with a translucent black
    [[NSColor colorWithCalibratedWhite:0.0 alpha:active ? 0.5 : 0.3] set];
    [[NSGraphicsContext currentContext] saveGraphicsState];
    NSShadow *shadow = [[[NSShadow alloc] init] autorelease];
    [shadow setShadowOffset:NSMakeSize(2, -1)];
    [shadow setShadowBlurRadius:2.0];
    [path fill];
    [[NSGraphicsContext currentContext] restoreGraphicsState];
  } else {
    gradient = getGradientForStyle(false, active);
    // Stroke with a translucent black
    [[NSColor colorWithCalibratedWhite:0.0 alpha:active ? 0.3 : 0.1] set];
  }

  [[NSGraphicsContext currentContext] saveGraphicsState];
  [[NSBezierPath bezierPathWithRect:NSOffsetRect(cellFrame, 0, -1)] addClip];
  [[NSColor colorWithCalibratedWhite:0.0 alpha:0.2] set];
  [path setLineWidth:selected ? 2.0 : 1.0];
  [path stroke];

  [[NSGraphicsContext currentContext] restoreGraphicsState];

  [gradient drawInBezierPath:path angle:90.0];

  if (!selected) {
    [path addClip];
    NSRect borderRect, contentRect;
    NSDivideRect(rect, &borderRect, &contentRect, 1, NSMaxYEdge);
    [[NSColor colorWithCalibratedWhite:0.0 alpha:0.4] set];
    NSRectFillUsingOperation(borderRect, NSCompositeSourceOver);
  }

  [[NSGraphicsContext currentContext] restoreGraphicsState];

  // Inset where the text and favicon are drawn to keep them away from the
  // sloping edges of the tab and the close box.
  int kInteriorInset = cellFrame.size.height / 2.0;
  NSRect frame = NSInsetRect(cellFrame, kInteriorInset, 0);
  frame.size.width -= 16; // Inset for close box
  [self drawInteriorWithFrame:frame
                       inView:controlView];
}

- (void)highlight:(BOOL)flag
        withFrame:(NSRect)cellFrame
           inView:(NSView *)controlView {
  // Don't do normal highlighting
}

@end
