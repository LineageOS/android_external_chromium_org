// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_PERMISSION_TYPES_H_
#define CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_PERMISSION_TYPES_H_

enum WebViewPermissionType {
  // Unknown type of permission request.
  WEB_VIEW_PERMISSION_TYPE_UNKNOWN,

  WEB_VIEW_PERMISSION_TYPE_DOWNLOAD,

  WEB_VIEW_PERMISSION_TYPE_FILESYSTEM,

  WEB_VIEW_PERMISSION_TYPE_GEOLOCATION,

  // JavaScript Dialogs: prompt, alert, confirm
  // Note: Even through dialogs do not use the permission API, the dialog API
  // is sufficiently similiar that it's convenient to consider it a permission
  // type for code reuse.
  WEB_VIEW_PERMISSION_TYPE_JAVASCRIPT_DIALOG,

  WEB_VIEW_PERMISSION_TYPE_LOAD_PLUGIN,

  // Media access (audio/video) permission request type.
  WEB_VIEW_PERMISSION_TYPE_MEDIA,

  // New window requests.
  // Note: Even though new windows don't use the permission API, the new window
  // API is sufficiently similar that it's convenient to consider it a
  // permission type for code reuse.
  WEB_VIEW_PERMISSION_TYPE_NEW_WINDOW,

  WEB_VIEW_PERMISSION_TYPE_POINTER_LOCK
};

#endif  // CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_PERMISSION_TYPES_H_
