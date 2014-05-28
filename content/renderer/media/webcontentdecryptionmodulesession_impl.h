// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_
#define CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "media/base/media_keys.h"
#include "third_party/WebKit/public/platform/WebContentDecryptionModuleSession.h"
#include "third_party/WebKit/public/platform/WebString.h"

namespace media {
class MediaKeys;
}

namespace content {
class CdmSessionAdapter;

class WebContentDecryptionModuleSessionImpl
    : public blink::WebContentDecryptionModuleSession {
 public:
  WebContentDecryptionModuleSessionImpl(
      uint32 session_id,
      Client* client,
      const scoped_refptr<CdmSessionAdapter>& adapter);
  virtual ~WebContentDecryptionModuleSessionImpl();

  // blink::WebContentDecryptionModuleSession implementation.
  virtual blink::WebString sessionId() const;
  virtual void initializeNewSession(const blink::WebString& mime_type,
                                    const uint8* init_data,
                                    size_t init_data_length);
  virtual void update(const uint8* response, size_t response_length);
  virtual void release();

  // Callbacks.
  void OnSessionCreated(const std::string& web_session_id);
  void OnSessionMessage(const std::vector<uint8>& message,
                        const GURL& destination_url);
  void OnSessionReady();
  void OnSessionClosed();
  void OnSessionError(media::MediaKeys::KeyError error_code,
                      uint32 system_code);

 private:
  scoped_refptr<CdmSessionAdapter> adapter_;

  // Non-owned pointer.
  Client* client_;

  // Web session ID is the app visible ID for this session generated by the CDM.
  // This value is not set until the CDM calls OnSessionCreated().
  blink::WebString web_session_id_;

  // Session ID is used to uniquely track this object so that CDM callbacks
  // can get routed to the correct object.
  const uint32 session_id_;

  DISALLOW_COPY_AND_ASSIGN(WebContentDecryptionModuleSessionImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_
