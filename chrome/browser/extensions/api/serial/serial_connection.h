// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SERIAL_SERIAL_CONNECTION_H_
#define CHROME_BROWSER_EXTENSIONS_API_SERIAL_SERIAL_CONNECTION_H_

#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/platform_file.h"
#include "chrome/browser/extensions/api/api_resource.h"
#include "net/base/io_buffer.h"

namespace extensions {

extern const char kSerialConnectionNotFoundError[];

class APIResourceEventNotifier;

// Encapsulates an open serial port. Platform-specific implementations are in
// _win and _posix versions of the the .cc file.
class SerialConnection : public APIResource {
 public:
  SerialConnection(const std::string& port,
                   int bitrate,
                   APIResourceEventNotifier* event_notifier);
  virtual ~SerialConnection();

  virtual bool Open();
  virtual void Close();
  virtual void Flush();

  virtual int Read(uint8* byte);
  virtual int Write(scoped_refptr<net::IOBuffer> io_buffer, int byte_count);

 protected:
  // Do platform-specific work after a successful Open().
  bool PostOpen();

 private:
  std::string port_;
  int bitrate_;
  base::PlatformFile file_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SERIAL_SERIAL_CONNECTION_H_
