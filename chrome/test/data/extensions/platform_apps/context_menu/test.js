// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.experimental.app.onLaunched.addListener(function() {
  chrome.contextMenus.create({
        title: 'Extension Item 1',
        onclick: function() {}
      },
      function() {
        chrome.windows.create({
            url: 'main.html',
            type: 'shell'
        });
      });
});
