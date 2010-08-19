// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_BACKGROUND_CONTENTS_H_
#define CHROME_BROWSER_TAB_CONTENTS_BACKGROUND_CONTENTS_H_
#pragma once

#include <string>

#include "chrome/browser/jsmessage_box_client.h"
#include "chrome/browser/renderer_host/render_view_host_delegate.h"
#include "chrome/browser/tab_contents/render_view_host_delegate_helper.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/view_types.h"
#include "chrome/common/window_container_type.h"
#include "webkit/glue/window_open_disposition.h"

class TabContents;
struct WebPreferences;

namespace gfx {
class Rect;
}

// This class is a peer of TabContents. It can host a renderer, but does not
// have any visible display. Its navigation is not managed by a
// NavigationController because is has no facility for navigating (other than
// programatically view window.location.href) or RenderViewHostManager because
// it is never allowed to navigate across a SiteInstance boundary.
class BackgroundContents : public RenderViewHostDelegate,
                           public RenderViewHostDelegate::View,
                           public NotificationObserver,
                           public JavaScriptMessageBoxClient {
 public:
  class Delegate {
   public:
    // Called by ShowCreatedWindow. Asks the delegate to attach the opened
    // TabContents to a suitable container (e.g. browser) or to show it if it's
    // a popup window.
    virtual void AddTabContents(TabContents* new_contents,
                                WindowOpenDisposition disposition,
                                const gfx::Rect& initial_pos,
                                bool user_gesture) = 0;

   protected:
    virtual ~Delegate() {}
  };

  BackgroundContents(SiteInstance* site_instance,
                     int routing_id,
                     Delegate* delegate);
  virtual ~BackgroundContents();

  // Provide access to the RenderViewHost for the
  // RenderViewHostDelegateViewHelper
  RenderViewHost* render_view_host() { return render_view_host_; }

  // RenderViewHostDelegate implementation.
  virtual RenderViewHostDelegate::View* GetViewDelegate() { return this; }
  virtual const GURL& GetURL() const { return url_; }
  virtual ViewType::Type GetRenderViewType() const {
    return ViewType::BACKGROUND_CONTENTS;
  }
  virtual int GetBrowserWindowID() const {
    return extension_misc::kUnknownWindowId;
  }
  virtual void DidNavigate(RenderViewHost* render_view_host,
                           const ViewHostMsg_FrameNavigate_Params& params);
  virtual WebPreferences GetWebkitPrefs();
  virtual void ProcessDOMUIMessage(const std::string& message,
                                   const ListValue* content,
                                   const GURL& source_url,
                                   int request_id,
                                   bool has_callback);
  virtual void RunJavaScriptMessage(const std::wstring& message,
                                    const std::wstring& default_prompt,
                                    const GURL& frame_url,
                                    const int flags,
                                    IPC::Message* reply_msg,
                                    bool* did_suppress_message);
  virtual void Close(RenderViewHost* render_view_host);
  virtual RendererPreferences GetRendererPrefs(Profile* profile) const;

  // RenderViewHostDelegate::View
  virtual void CreateNewWindow(
      int route_id,
      WindowContainerType window_container_type,
      const string16& frame_name);
  virtual void CreateNewWidget(int route_id, WebKit::WebPopupType popup_type);
  virtual void CreateNewFullscreenWidget(
      int route_id, WebKit::WebPopupType popup_type);
  virtual void ShowCreatedWindow(int route_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_pos,
                                 bool user_gesture);
  virtual void ShowCreatedWidget(int route_id,
                                 const gfx::Rect& initial_pos);
  virtual void ShowCreatedFullscreenWidget(int route_id);
  virtual void ShowContextMenu(const ContextMenuParams& params) {}
  virtual void StartDragging(const WebDropData& drop_data,
                             WebKit::WebDragOperationsMask allowed_operations,
                             const SkBitmap& image,
                             const gfx::Point& image_offset) {}
  virtual void UpdateDragCursor(WebKit::WebDragOperation operation) {}
  virtual void GotFocus() {}
  virtual void TakeFocus(bool reverse) {}
  virtual void Activate() {}
  virtual void Deactivate() {}
  virtual bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                                      bool* is_keyboard_shortcut) {
    return false;
  }
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event) {}
  virtual void HandleMouseMove() {}
  virtual void HandleMouseDown() {}
  virtual void HandleMouseLeave() {}
  virtual void UpdatePreferredSize(const gfx::Size& new_size) {}

  // NotificationObserver
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // JavaScriptMessageBoxClient
  virtual std::wstring GetMessageBoxTitle(const GURL& frame_url,
                                          bool is_alert);
  virtual gfx::NativeWindow GetMessageBoxRootWindow();
  virtual void OnMessageBoxClosed(IPC::Message* reply_msg,
                                  bool success,
                                  const std::wstring& prompt);
  virtual void SetSuppressMessageBoxes(bool suppress_message_boxes) {}
  virtual TabContents* AsTabContents() { return NULL; }
  virtual ExtensionHost* AsExtensionHost() { return NULL; }

 protected:
  // Exposed for testing.
  BackgroundContents();

 private:
  // The delegate for this BackgroundContents.
  Delegate* delegate_;

  // The host for our HTML content.
  RenderViewHost* render_view_host_;

  // Common implementations of some RenderViewHostDelegate::View methods.
  RenderViewHostDelegateViewHelper delegate_view_helper_;

  // The URL being hosted.
  GURL url_;

  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundContents);
};

// This is the data sent out as the details with BACKGROUND_CONTENTS_OPENED.
struct BackgroundContentsOpenedDetails {
  // The BackgroundContents object that has just been opened.
  BackgroundContents* contents;

  // The name of the parent frame for these contents.
  const string16& frame_name;

  // The ID of the parent application (if any).
  const string16& application_id;
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_BACKGROUND_CONTENTS_H_
