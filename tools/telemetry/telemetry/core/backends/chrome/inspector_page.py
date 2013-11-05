# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import logging
import time

class InspectorPage(object):
  def __init__(self, inspector_backend):
    self._inspector_backend = inspector_backend
    self._inspector_backend.RegisterDomain(
        'Page',
        self._OnNotification,
        self._OnClose)
    self._navigation_pending = False
    self._navigation_url = ""

  def _OnNotification(self, msg):
    logging.debug('Notification: %s', json.dumps(msg, indent=2))
    if msg['method'] == 'Page.frameNavigated' and self._navigation_pending:
      url = msg['params']['frame']['url']
      if (self._navigation_url == url or
          (not url == 'chrome://newtab/' and not url == 'about:blank'
          and not 'parentId' in msg['params']['frame'])):
        # Marks the navigation as complete and unblocks the
        # PerformActionAndWaitForNavigate call.
        self._navigation_pending = False

  def _OnClose(self):
    pass

  def _EnablePageNotifications(self):
    request = {
        'method': 'Page.enable'
        }
    res = self._inspector_backend.SyncRequest(request)
    assert len(res['result'].keys()) == 0

  def _DisablePageNotifications(self):
    request = {
        'method': 'Page.disable'
        }
    res = self._inspector_backend.SyncRequest(request)
    assert len(res['result'].keys()) == 0

  def PerformActionAndWaitForNavigate(self, action_function, timeout=60):
    """Executes action_function, and waits for the navigation to complete.

    action_function is expect to result in a navigation. This function returns
    when the navigation is complete or when the timeout has been exceeded.
    """
    self._EnablePageNotifications()
    try:
      action_function()
      start_time = time.time()
      remaining_time = timeout
      self._navigation_pending = True
      while self._navigation_pending and remaining_time > 0:
        remaining_time = max(timeout - (time.time() - start_time), 0.0)
        self._inspector_backend.DispatchNotifications(remaining_time)
    finally:
      self._DisablePageNotifications()

  def Navigate(self, url, script_to_evaluate_on_commit=None, timeout=60):
    """Navigates to |url|.

    If |script_to_evaluate_on_commit| is given, the script source string will be
    evaluated when the navigation is committed. This is after the context of
    the page exists, but before any script on the page itself has executed.
    """

    def DoNavigate():
      if script_to_evaluate_on_commit:
        request = {
            'method': 'Page.addScriptToEvaluateOnLoad',
            'params': {
                'scriptSource': script_to_evaluate_on_commit,
                }
            }
        self._inspector_backend.SyncRequest(request)
      # Navigate the page. However, there seems to be a bug in chrome devtools
      # protocol where the request id for this event gets held on the browser
      # side pretty much indefinitely.
      #
      # So, instead of waiting for the event to actually complete, wait for the
      # Page.frameNavigated event.
      request = {
          'method': 'Page.navigate',
          'params': {
              'url': url,
              }
          }
      self._inspector_backend.SendAndIgnoreResponse(request)
    self._navigation_url = url
    self.PerformActionAndWaitForNavigate(DoNavigate, timeout)

  def GetCookieByName(self, name, timeout=60):
    """Returns the value of the cookie by the given |name|."""
    request = {
        'method': 'Page.getCookies'
        }
    res = self._inspector_backend.SyncRequest(request, timeout)
    cookies = res['result']['cookies']
    for cookie in cookies:
      if cookie['name'] == name:
        return cookie['value']
    return None

  def CollectGarbage(self, timeout=60):
    request = {
        'method': 'HeapProfiler.CollectGarbage'
        }
    self._inspector_backend.SyncRequest(request, timeout)
