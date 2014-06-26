// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_

#include <map>
#include <string>

#include "base/scoped_observer.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_registry_observer.h"

namespace content {
class BrowserContext;
class StreamHandle;
}

namespace extensions {
class ExtensionRegistry;

class StreamsPrivateAPI : public BrowserContextKeyedAPI,
                          public ExtensionRegistryObserver {
 public:
  // Convenience method to get the StreamsPrivateAPI for a BrowserContext.
  static StreamsPrivateAPI* Get(content::BrowserContext* context);

  explicit StreamsPrivateAPI(content::BrowserContext* context);
  virtual ~StreamsPrivateAPI();

  // Send the onExecuteMimeTypeHandler event to |extension_id|.
  // |web_contents| is used to determine the tabId where the document is being
  // opened. The data for the document will be readable from |stream|, and
  // should be |expected_content_size| bytes long. If the viewer is being opened
  // in a BrowserPlugin, specify a non-empty |view_id| of the plugin.
  void ExecuteMimeTypeHandler(const std::string& extension_id,
                              content::WebContents* web_contents,
                              scoped_ptr<content::StreamHandle> stream,
                              const std::string& view_id,
                              int64 expected_content_size);

  void AbortStream(const std::string& extension_id,
                   const GURL& stream_url,
                   const base::Closure& callback);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<StreamsPrivateAPI>* GetFactoryInstance();

 private:
  friend class BrowserContextKeyedAPIFactory<StreamsPrivateAPI>;
  typedef std::map<std::string,
                   std::map<GURL,
                            linked_ptr<content::StreamHandle> > > StreamMap;

  // ExtensionRegistryObserver implementation.
  virtual void OnExtensionUnloaded(
      content::BrowserContext* browser_context,
      const Extension* extension,
      UnloadedExtensionInfo::Reason reason) OVERRIDE;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "StreamsPrivateAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  content::BrowserContext* const browser_context_;
  StreamMap streams_;
  base::WeakPtrFactory<StreamsPrivateAPI> weak_ptr_factory_;

  // Listen to extension unloaded notifications.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;
};

class StreamsPrivateAbortFunction : public UIThreadExtensionFunction {
 public:
  StreamsPrivateAbortFunction();
  DECLARE_EXTENSION_FUNCTION("streamsPrivate.abort", STREAMSPRIVATE_ABORT)

 protected:
  virtual ~StreamsPrivateAbortFunction() {}

  // ExtensionFunction:
  virtual ExtensionFunction::ResponseAction Run() OVERRIDE;

 private:
  void OnClose();

  std::string stream_url_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
