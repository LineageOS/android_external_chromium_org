// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTENT_CLIENT_VIEWS_CONTENT_CLIENT_MAIN_PARTS_AURA_H_
#define UI_VIEWS_CONTENT_CLIENT_VIEWS_CONTENT_CLIENT_MAIN_PARTS_AURA_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "ui/views_content_client/views_content_client_main_parts.h"

namespace wm {
class WMState;
}

namespace ui {

class ViewsContentClientMainPartsAura : public ViewsContentClientMainParts {
 protected:
  ViewsContentClientMainPartsAura(
      const content::MainFunctionParams& content_params,
      ViewsContentClient* views_content_client);
  virtual ~ViewsContentClientMainPartsAura();

  // content::BrowserMainParts:
  virtual void ToolkitInitialized() OVERRIDE;
  virtual void PostMainMessageLoopRun() OVERRIDE;

 private:
  scoped_ptr< ::wm::WMState> wm_state_;

  DISALLOW_COPY_AND_ASSIGN(ViewsContentClientMainPartsAura);
};

}  // namespace ui

#endif  // UI_VIEWS_CONTENT_CLIENT_VIEWS_CONTENT_CLIENT_MAIN_PARTS_AURA_H_
