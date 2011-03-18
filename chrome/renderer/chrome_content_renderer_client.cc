// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_content_renderer_client.h"

#include "base/command_line.h"
#include "base/values.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/jstemplate_builder.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/url_constants.h"
#include "chrome/renderer/blocked_plugin.h"
#include "chrome/renderer/localized_error.h"
#include "chrome/renderer/render_thread.h"
#include "chrome/renderer/render_view.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "grit/renderer_resources.h"
#include "net/base/net_errors.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebPluginParams.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebURL.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebURLError.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebURLRequest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/plugins/npapi/plugin_list.h"
#include "webkit/plugins/ppapi/plugin_module.h"

using WebKit::WebFrame;
using WebKit::WebPlugin;
using WebKit::WebPluginParams;
using WebKit::WebString;
using WebKit::WebURLError;
using WebKit::WebURLRequest;
using WebKit::WebURLResponse;

namespace chrome {

SkBitmap* ChromeContentRendererClient::GetSadPluginBitmap() {
  return ResourceBundle::GetSharedInstance().GetBitmapNamed(IDR_SAD_PLUGIN);
}

std::string ChromeContentRendererClient::GetDefaultEncoding() {
  return l10n_util::GetStringUTF8(IDS_DEFAULT_ENCODING);
}

WebPlugin* ChromeContentRendererClient::CreatePlugin(
      RenderView* render_view,
      WebFrame* frame,
      const WebPluginParams& params) {
  bool found = false;
  ContentSetting plugin_setting = CONTENT_SETTING_DEFAULT;
  CommandLine* cmd = CommandLine::ForCurrentProcess();
  webkit::npapi::WebPluginInfo info;
  GURL url(params.url);
  std::string actual_mime_type;
  render_view->Send(new ViewHostMsg_GetPluginInfo(
      render_view->routing_id(), url, frame->top()->url(),
      params.mimeType.utf8(), &found, &info, &plugin_setting,
      &actual_mime_type));

  if (!found)
    return NULL;
  DCHECK(plugin_setting != CONTENT_SETTING_DEFAULT);
  if (!webkit::npapi::IsPluginEnabled(info))
    return NULL;

  const webkit::npapi::PluginGroup* group =
      webkit::npapi::PluginList::Singleton()->GetPluginGroup(info);
  DCHECK(group != NULL);

  if (group->IsVulnerable() &&
      !cmd->HasSwitch(switches::kAllowOutdatedPlugins)) {
    render_view->Send(new ViewHostMsg_BlockedOutdatedPlugin(
        render_view->routing_id(), group->GetGroupName(),
        GURL(group->GetUpdateURL())));
    return CreatePluginPlaceholder(
        render_view, frame, params, *group, IDR_BLOCKED_PLUGIN_HTML,
        IDS_PLUGIN_OUTDATED, false);
  }

  ContentSetting host_setting = render_view->current_content_settings_.
      settings[CONTENT_SETTINGS_TYPE_PLUGINS];

  if (group->RequiresAuthorization() &&
      !cmd->HasSwitch(switches::kAlwaysAuthorizePlugins) &&
      (plugin_setting == CONTENT_SETTING_ALLOW ||
       plugin_setting == CONTENT_SETTING_ASK) &&
      host_setting == CONTENT_SETTING_DEFAULT) {
    render_view->Send(new ViewHostMsg_BlockedOutdatedPlugin(
        render_view->routing_id(), group->GetGroupName(), GURL()));
    return CreatePluginPlaceholder(
        render_view, frame, params, *group, IDR_BLOCKED_PLUGIN_HTML,
        IDS_PLUGIN_NOT_AUTHORIZED, false);
  }

  if (info.path.value() == webkit::npapi::kDefaultPluginLibraryName ||
      plugin_setting == CONTENT_SETTING_ALLOW ||
      host_setting == CONTENT_SETTING_ALLOW) {
    // Delay loading plugins if prerendering.
    if (render_view->is_prerendering_) {
      return CreatePluginPlaceholder(
          render_view, frame, params, *group, IDR_CLICK_TO_PLAY_PLUGIN_HTML,
          IDS_PLUGIN_LOAD, true);
    }

    scoped_refptr<webkit::ppapi::PluginModule> pepper_module(
        render_view->pepper_delegate_.CreatePepperPlugin(info.path));
    if (pepper_module) {
      return render_view->CreatePepperPlugin(
          frame, params, info.path, pepper_module.get());
    }

    return render_view->CreateNPAPIPlugin(
        frame, params, info.path, actual_mime_type);
  }

  std::string resource;
  if (cmd->HasSwitch(switches::kEnableResourceContentSettings))
    resource = group->identifier();
  render_view->DidBlockContentType(CONTENT_SETTINGS_TYPE_PLUGINS, resource);
  if (plugin_setting == CONTENT_SETTING_ASK) {
    return CreatePluginPlaceholder(
        render_view, frame, params, *group, IDR_CLICK_TO_PLAY_PLUGIN_HTML,
        IDS_PLUGIN_LOAD, false);
  } else {
    return CreatePluginPlaceholder(
        render_view, frame, params, *group, IDR_BLOCKED_PLUGIN_HTML,
        IDS_PLUGIN_BLOCKED, false);
  }
}

WebPlugin* ChromeContentRendererClient::CreatePluginPlaceholder(
    RenderView* render_view,
    WebFrame* frame,
    const WebPluginParams& params,
    const webkit::npapi::PluginGroup& group,
    int resource_id,
    int message_id,
    bool is_blocked_for_prerendering) {
  // |blocked_plugin| will delete itself when the WebViewPlugin
  // is destroyed.
  BlockedPlugin* blocked_plugin =
      new BlockedPlugin(render_view,
                        frame,
                        group,
                        params,
                        render_view->webkit_preferences(),
                        resource_id,
                        l10n_util::GetStringFUTF16(message_id,
                                                   group.GetGroupName()),
                        is_blocked_for_prerendering);
  return blocked_plugin->plugin();
}

std::string ChromeContentRendererClient::GetNavigationErrorHtml(
    const WebURLRequest& failed_request,
    const WebURLError& error) {
  GURL failed_url = error.unreachableURL;
  std::string html;
  const Extension* extension = NULL;

  // Use a local error page.
  int resource_id;
  DictionaryValue error_strings;
  if (failed_url.is_valid() && !failed_url.SchemeIs(chrome::kExtensionScheme))
    extension = RenderThread::current()->GetExtensions()->GetByURL(failed_url);
  if (extension) {
    LocalizedError::GetAppErrorStrings(error, failed_url, extension,
                                       &error_strings);

    // TODO(erikkay): Should we use a different template for different
    // error messages?
    resource_id = IDR_ERROR_APP_HTML;
  } else {
    if (error.domain == WebString::fromUTF8(net::kErrorDomain) &&
        error.reason == net::ERR_CACHE_MISS &&
        EqualsASCII(failed_request.httpMethod(), "POST")) {
      LocalizedError::GetFormRepostStrings(failed_url, &error_strings);
    } else {
      LocalizedError::GetStrings(error, &error_strings);
    }
    resource_id = IDR_NET_ERROR_HTML;
  }

  const base::StringPiece template_html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id));
  if (template_html.empty()) {
    NOTREACHED() << "unable to load template. ID: " << resource_id;
  } else {
    // "t" is the id of the templates root node.
    html = jstemplate_builder::GetTemplatesHtml(
        template_html, &error_strings, "t");
  }

  return html;
}

}  // namespace chrome
