// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_TAB_CONTENTS_CONTROLLER_H_
#define CHROME_BROWSER_COCOA_TAB_CONTENTS_CONTROLLER_H_

#include <Cocoa/Cocoa.h>

class TabContents;
class TabContentsCommandObserver;
class TabStripModel;

// A class that controls the web contents of a tab. It manages displaying the
// native view for a given TabContents in |contentsBox_|.

@interface TabContentsController : NSViewController {
 @private
  TabContentsCommandObserver* observer_;  // nil if |commands_| is nil
  TabContents* contents_;  // weak

  IBOutlet NSBox* contentsBox_;
}

// Create the contents of a tab represented by |contents| and loaded from the
// nib given by |name|.
- (id)initWithNibName:(NSString*)name
             contents:(TabContents*)contents;

// Take this view (toolbar and web contents) full screen
- (IBAction)fullScreen:(id)sender;

// Called when the tab contents is about to be put into the view hierarchy as
// the selected tab. Handles things such as ensuring the toolbar is correctly
// enabled.
- (void)willBecomeSelectedTab;

// Called when the tab contents is updated in some non-descript way (the
// notification from the model isn't specific). |updatedContents| could reflect
// an entirely new tab contents object.
- (void)tabDidChange:(TabContents*)updatedContents;

@end

#endif  // CHROME_BROWSER_COCOA_TAB_CONTENTS_CONTROLLER_H_
