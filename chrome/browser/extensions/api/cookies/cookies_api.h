// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines the Chrome Extensions Cookies API functions for accessing internet
// cookies, as specified in the extension API JSON.

#ifndef CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/browser/net/chrome_cookie_notification_details.h"
#include "chrome/common/extensions/api/cookies.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "net/cookies/canonical_cookie.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace extensions {

// Observes CookieMonster notifications and routes them as events to the
// extension system.
class CookiesEventRouter : public content::NotificationObserver {
 public:
  explicit CookiesEventRouter(content::BrowserContext* context);
  virtual ~CookiesEventRouter();

 private:
  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Handler for the COOKIE_CHANGED event. The method takes the details of such
  // an event and constructs a suitable JSON formatted extension event from it.
  void CookieChanged(Profile* profile, ChromeCookieDetails* details);

  // This method dispatches events to the extension message service.
  void DispatchEvent(content::BrowserContext* context,
                     const std::string& event_name,
                     scoped_ptr<base::ListValue> event_args,
                     GURL& cookie_domain);

  // Used for tracking registrations to CookieMonster notifications.
  content::NotificationRegistrar registrar_;

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(CookiesEventRouter);
};

// Implements the cookies.get() extension function.
class CookiesGetFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.get", COOKIES_GET)

  CookiesGetFunction();

 protected:
  virtual ~CookiesGetFunction();

  // ExtensionFunction:
  virtual bool RunAsync() OVERRIDE;

 private:
  void GetCookieOnIOThread();
  void RespondOnUIThread();
  void GetCookieCallback(const net::CookieList& cookie_list);

  GURL url_;
  scoped_refptr<net::URLRequestContextGetter> store_browser_context_;
  scoped_ptr<extensions::api::cookies::Get::Params> parsed_args_;
};

// Implements the cookies.getAll() extension function.
class CookiesGetAllFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.getAll", COOKIES_GETALL)

  CookiesGetAllFunction();

 protected:
  virtual ~CookiesGetAllFunction();

  // ExtensionFunction:
  virtual bool RunAsync() OVERRIDE;

 private:
  void GetAllCookiesOnIOThread();
  void RespondOnUIThread();
  void GetAllCookiesCallback(const net::CookieList& cookie_list);

  GURL url_;
  scoped_refptr<net::URLRequestContextGetter> store_browser_context_;
  scoped_ptr<extensions::api::cookies::GetAll::Params> parsed_args_;
};

// Implements the cookies.set() extension function.
class CookiesSetFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.set", COOKIES_SET)

  CookiesSetFunction();

 protected:
  virtual ~CookiesSetFunction();
  virtual bool RunAsync() OVERRIDE;

 private:
  void SetCookieOnIOThread();
  void RespondOnUIThread();
  void PullCookie(bool set_cookie_);
  void PullCookieCallback(const net::CookieList& cookie_list);

  GURL url_;
  bool success_;
  scoped_refptr<net::URLRequestContextGetter> store_browser_context_;
  scoped_ptr<extensions::api::cookies::Set::Params> parsed_args_;
};

// Implements the cookies.remove() extension function.
class CookiesRemoveFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.remove", COOKIES_REMOVE)

  CookiesRemoveFunction();

 protected:
  virtual ~CookiesRemoveFunction();

  // ExtensionFunction:
  virtual bool RunAsync() OVERRIDE;

 private:
  void RemoveCookieOnIOThread();
  void RespondOnUIThread();
  void RemoveCookieCallback();

  GURL url_;
  scoped_refptr<net::URLRequestContextGetter> store_browser_context_;
  scoped_ptr<extensions::api::cookies::Remove::Params> parsed_args_;
};

// Implements the cookies.getAllCookieStores() extension function.
class CookiesGetAllCookieStoresFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.getAllCookieStores",
                             COOKIES_GETALLCOOKIESTORES)

 protected:
  virtual ~CookiesGetAllCookieStoresFunction() {}

  // ExtensionFunction:
  virtual bool RunSync() OVERRIDE;
};

class CookiesAPI : public BrowserContextKeyedAPI,
                   public extensions::EventRouter::Observer {
 public:
  explicit CookiesAPI(content::BrowserContext* context);
  virtual ~CookiesAPI();

  // KeyedService implementation.
  virtual void Shutdown() OVERRIDE;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<CookiesAPI>* GetFactoryInstance();

  // EventRouter::Observer implementation.
  virtual void OnListenerAdded(const extensions::EventListenerInfo& details)
      OVERRIDE;

 private:
  friend class BrowserContextKeyedAPIFactory<CookiesAPI>;

  content::BrowserContext* browser_context_;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "CookiesAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;

  // Created lazily upon OnListenerAdded.
  scoped_ptr<CookiesEventRouter> cookies_event_router_;

  DISALLOW_COPY_AND_ASSIGN(CookiesAPI);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
