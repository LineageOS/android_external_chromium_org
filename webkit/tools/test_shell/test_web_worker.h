// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_TOOLS_TEST_SHELL_TEST_WEB_WORKER_H_
#define WEBKIT_TOOLS_TEST_SHELL_TEST_WEB_WORKER_H_

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "third_party/WebKit/WebKit/chromium/public/WebMessagePortChannel.h"
#include "third_party/WebKit/WebKit/chromium/public/WebWorker.h"
#include "third_party/WebKit/WebKit/chromium/public/WebWorkerClient.h"

namespace WebKit {
class WebApplicationCacheHost;
class WebApplicationCacheHostClient;
class WebNotificationPresenter;
class WebString;
class WebURL;
}

// WebWorkers are not functional in test_shell. This class effectively
// stubs things out.
class TestWebWorker : public WebKit::WebWorker,
                      public WebKit::WebWorkerClient,
                      public base::RefCounted<TestWebWorker> {
 public:
  TestWebWorker() {
    AddRef();  // Adds the reference held for worker object.
    AddRef();  // Adds the reference held for worker context object.
  }

  // WebWorker methods:
  virtual void startWorkerContext(const WebKit::WebURL& script_url,
                                  const WebKit::WebString& user_agent,
                                  const WebKit::WebString& source_code) {
  }
  virtual void terminateWorkerContext() {
  }
  virtual void postMessageToWorkerContext(
      const WebKit::WebString& message,
      const WebKit::WebMessagePortChannelArray& channel) {
  }
  virtual void workerObjectDestroyed() {
    Release();  // Releases the reference held for worker object.
  }
  virtual void clientDestroyed() {
  }

  // WebWorkerClient methods:
  virtual void postMessageToWorkerObject(
      const WebKit::WebString& message,
      const WebKit::WebMessagePortChannelArray& channel) {
  }
  virtual void postExceptionToWorkerObject(
      const WebKit::WebString& error_message,
      int line_number,
      const WebKit::WebString& source_url) {
  }
  virtual void postConsoleMessageToWorkerObject(
      int destination_id,
      int source_id,
      int message_type,
      int message_level,
      const WebKit::WebString& message,
      int line_number,
      const WebKit::WebString& source_url) {
  }
  virtual void confirmMessageFromWorkerObject(bool has_pending_activity) { }
  virtual void reportPendingActivity(bool has_pending_activity) { }
  virtual void workerContextClosed() { }
  virtual void workerContextDestroyed() {
    Release();    // Releases the reference held for worker context object.
  }
  virtual WebKit::WebWorker* createWorker(WebKit::WebWorkerClient* client) {
    return NULL;
  }
  virtual WebKit::WebNotificationPresenter* notificationPresenter() {
    return NULL;
  }
  virtual WebKit::WebApplicationCacheHost* createApplicationCacheHost(
      WebKit::WebApplicationCacheHostClient*) {
    return NULL;
  }

 private:
  friend class base::RefCounted<TestWebWorker>;

  ~TestWebWorker() {}

  DISALLOW_COPY_AND_ASSIGN(TestWebWorker);
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_TEST_WEB_WORKER_H_
