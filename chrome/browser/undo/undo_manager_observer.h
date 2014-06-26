// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UNDO_UNDO_MANAGER_OBSERVER_H_
#define CHROME_BROWSER_UNDO_UNDO_MANAGER_OBSERVER_H_

// Observer for the UndoManager.
class UndoManagerObserver {
 public:
  // Invoked when the internal state of the UndoManager has changed.
  virtual void OnUndoManagerStateChange() = 0;

 protected:
  virtual ~UndoManagerObserver() {}
};

#endif  // CHROME_BROWSER_UNDO_UNDO_MANAGER_OBSERVER_H_
