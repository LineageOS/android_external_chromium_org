// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/gtk/notifications/balloon_view_host_gtk.h"

#include "chrome/browser/notifications/balloon.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host_view_gtk.h"
#include "content/public/browser/web_contents.h"

BalloonViewHost::BalloonViewHost(Balloon* balloon)
    : BalloonHost(balloon) {
}

BalloonViewHost::~BalloonViewHost() {
  Shutdown();
}

void BalloonViewHost::UpdateActualSize(const gfx::Size& new_size) {
  RenderViewHost* host = web_contents_->GetRenderViewHost();
  if (host) {
    RenderWidgetHostView* view = host->view();
    if (view)
      view->SetSize(new_size);
  }

  gtk_widget_set_size_request(
      native_view(), new_size.width(), new_size.height());
}

gfx::NativeView BalloonViewHost::native_view() const {
  return web_contents_->GetNativeView();
}
