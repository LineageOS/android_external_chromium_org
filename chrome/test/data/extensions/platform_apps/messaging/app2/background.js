// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onConnectExternal.addListener(function(port) {
  port.onMessage.addListener(function(msg) {
    if (msg == 'ok_to_disconnect') {
      port.disconnect();
    } else {
      port.postMessage(msg + '_reply');
    }
  });
});

chrome.runtime.onMessageExternal.addListener(function(msg, sender, callback) {
  if (msg == 'hello')
    callback('hello_response');
  else
    callback();
});
