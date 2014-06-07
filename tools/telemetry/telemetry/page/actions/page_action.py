# Copyright 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page.actions import wait_until

class PageActionNotSupported(Exception):
  pass

class PageActionFailed(Exception):
  pass


class PageAction(object):
  """Represents an action that a user might try to perform to a page."""

  def __init__(self, attributes=None):
    if attributes:
      for k, v in attributes.iteritems():
        setattr(self, k, v)
    if hasattr(self, 'wait_until'):
      self.wait_until = wait_until.WaitUntil(self, self.wait_until)
    else:
      self.wait_until = None

  def WillRunAction(self, tab):
    """Override to do action-specific setup before
    Test.WillRunAction is called."""
    pass

  def WillWaitAfterRun(self):
    return self.wait_until is not None

  def RunActionAndMaybeWait(self, tab):
    if self.wait_until:
      self.wait_until.RunActionAndWait(tab)
    else:
      self.RunAction(tab)

  def RunAction(self, tab):
    raise NotImplementedError()

  def CleanUp(self, tab):
    pass
