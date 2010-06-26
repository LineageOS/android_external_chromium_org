// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "base/cocoa_protocols_mac.h"
#include "base/scoped_nsobject.h"
#include "base/scoped_ptr.h"

@class InfoBubbleView;

// Base class for bubble controllers. Manages a xib that contains an
// InfoBubbleWindow which contains an InfoBubbleView. Contains code to close
// the bubble window on clicks outside of the window, and the like.
// To use this class:
// 1. Create a new xib that contains a window. Change the window's class to
//    InfoBubbleWindow. Give it a child view that autosizes to the window's full
//    size, give it class InfoBubbleView. Make the controller the window's
//    delegate.
// 2. Create a subclass of BaseBubbleController.
// 3. Change the xib's File Owner to your subclass.
// 4. Hook up the File Owner's |bubble_| to the InfoBubbleView in the xib.
@interface BaseBubbleController : NSWindowController<NSWindowDelegate> {
 @private
  NSWindow* parentWindow_;  // weak
  NSPoint anchor_;
  IBOutlet InfoBubbleView* bubble_;  // to set arrow position
}

// Creates a bubble. |nibPath| is just the basename, e.g. @"FirstRunBubble".
// |anchoredAt| is in screen space. You need to call -showWindow: to make the
// bubble visible. It will autorelease itself when the user dismisses the
// bubble.
// This is the designated initializer.
- (id)initWithWindowNibPath:(NSString*)nibPath
               parentWindow:(NSWindow*)parentWindow
                 anchoredAt:(NSPoint)anchoredAt;


// Creates a bubble. |nibPath| is just the basename, e.g. @"FirstRunBubble".
// |view| must be in a window. The bubble will point at |offset| relative to
// |view|'s lower left corner. You need to call -showWindow: to make the
// bubble visible. It will autorelease itself when the user dismisses the
// bubble.
- (id)initWithWindowNibPath:(NSString*)nibPath
             relativeToView:(NSView*)view
                     offset:(NSPoint)offset;


@property (nonatomic, readonly) InfoBubbleView* bubble;

@end
