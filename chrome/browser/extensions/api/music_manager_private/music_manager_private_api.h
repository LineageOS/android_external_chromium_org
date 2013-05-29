// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MUSIC_MANAGER_PRIVATE_MUSIC_MANAGER_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_MUSIC_MANAGER_PRIVATE_MUSIC_MANAGER_PRIVATE_API_H_

#include "chrome/browser/extensions/extension_function.h"

namespace extensions {

namespace api {

class MusicManagerPrivateGetDeviceIdFunction : public SyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("musicManagerPrivate.getDeviceId",
                             MUSICMANAGERPRIVATE_GETDEVICEID)

  MusicManagerPrivateGetDeviceIdFunction();

 protected:
  virtual ~MusicManagerPrivateGetDeviceIdFunction();

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

} // namespace api

} // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MUSIC_MANAGER_PRIVATE_MUSIC_MANAGER_PRIVATE_API_H_
