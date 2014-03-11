// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/content_renderer_client.h"

#include "components/plugins/renderer/mobile_youtube_plugin.h"
#include "content/public/common/content_constants.h"
#include "grit/content_resources.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "ui/base/resource/resource_bundle.h"

namespace content {

SkBitmap* ContentRendererClient::GetSadPluginBitmap() {
  return NULL;
}

SkBitmap* ContentRendererClient::GetSadWebViewBitmap() {
  return NULL;
}

std::string ContentRendererClient::GetDefaultEncoding() {
  return std::string();
}

bool ContentRendererClient::OverrideCreatePlugin(
    RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {

  *plugin = CreatePlugin(render_frame, frame, params);
  return true;
}

//SWE-feature-youtube-plugin
// CreatePlugin previously was the stop-point in rendering plugins
// (returned NULL). Added handling of Flash-YouTube specific plugins only
blink::WebPlugin* ContentRendererClient::CreatePlugin(
    RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& original_params) {
  GURL url(original_params.url);
  std::string orig_mime_type = original_params.mimeType.utf8();

#if defined(OS_ANDROID)
  if (plugins::MobileYouTubePlugin::IsYouTubeURL(url, orig_mime_type)) {
    base::StringPiece template_html(
        ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_MOBILE_YOUTUBE_PLUGIN_HTML));

    return (new plugins::MobileYouTubePlugin(
                render_frame,
                frame,
                original_params,
                template_html,
                GURL(kPluginPlaceholderDataURL)))->plugin();
  }
#endif

  return NULL;
}
//SWE-feature-youtube-plugin

blink::WebPlugin* ContentRendererClient::CreatePluginReplacement(
    RenderFrame* render_frame,
    const base::FilePath& plugin_path) {
  return NULL;
}

bool ContentRendererClient::HasErrorPage(int http_status_code,
                                         std::string* error_domain) {
  return false;
}

bool ContentRendererClient::ShouldSuppressErrorPage(RenderFrame* render_frame,
                                                    const GURL& url) {
  return false;
}

void ContentRendererClient::DeferMediaLoad(RenderFrame* render_frame,
                                           const base::Closure& closure) {
  closure.Run();
}

blink::WebMediaStreamCenter*
ContentRendererClient::OverrideCreateWebMediaStreamCenter(
    blink::WebMediaStreamCenterClient* client) {
  return NULL;
}

blink::WebRTCPeerConnectionHandler*
ContentRendererClient::OverrideCreateWebRTCPeerConnectionHandler(
    blink::WebRTCPeerConnectionHandlerClient* client) {
  return NULL;
}

blink::WebMIDIAccessor*
ContentRendererClient::OverrideCreateMIDIAccessor(
    blink::WebMIDIAccessorClient* client) {
  return NULL;
}

blink::WebAudioDevice*
ContentRendererClient::OverrideCreateAudioDevice(
    double sample_rate) {
  return NULL;
}

blink::WebClipboard* ContentRendererClient::OverrideWebClipboard() {
  return NULL;
}

blink::WebThemeEngine* ContentRendererClient::OverrideThemeEngine() {
  return NULL;
}

blink::WebSpeechSynthesizer* ContentRendererClient::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return NULL;
}

bool ContentRendererClient::RunIdleHandlerWhenWidgetsHidden() {
  return true;
}

bool ContentRendererClient::AllowPopup() {
  return false;
}

#ifdef OS_ANDROID
bool ContentRendererClient::HandleNavigation(
    RenderFrame* render_frame,
    DocumentState* document_state,
    int opener_id,
    blink::WebFrame* frame,
    const blink::WebURLRequest& request,
    blink::WebNavigationType type,
    blink::WebNavigationPolicy default_policy,
    bool is_redirect) {
  return false;
}
#endif

bool ContentRendererClient::ShouldFork(blink::WebFrame* frame,
                                       const GURL& url,
                                       const std::string& http_method,
                                       bool is_initial_navigation,
                                       bool is_server_redirect,
                                       bool* send_referrer) {
  return false;
}

bool ContentRendererClient::WillSendRequest(
    blink::WebFrame* frame,
    PageTransition transition_type,
    const GURL& url,
    const GURL& first_party_for_cookies,
    GURL* new_url) {
  return false;
}

unsigned long long ContentRendererClient::VisitedLinkHash(
    const char* canonical_url, size_t length) {
  return 0LL;
}

bool ContentRendererClient::IsLinkVisited(unsigned long long link_hash) {
  return false;
}

blink::WebPrescientNetworking*
ContentRendererClient::GetPrescientNetworking() {
  return NULL;
}

bool ContentRendererClient::ShouldOverridePageVisibilityState(
    const RenderFrame* render_frame,
    blink::WebPageVisibilityState* override_state) {
  return false;
}

const void* ContentRendererClient::CreatePPAPIInterface(
    const std::string& interface_name) {
  return NULL;
}

bool ContentRendererClient::IsExternalPepperPlugin(
    const std::string& module_name) {
  return false;
}

bool ContentRendererClient::AllowPepperMediaStreamAPI(const GURL& url) {
  return false;
}

void ContentRendererClient::AddKeySystems(
    std::vector<KeySystemInfo>* key_systems) {
}

bool ContentRendererClient::ShouldReportDetailedMessageForSource(
    const base::string16& source) const {
  return false;
}

bool ContentRendererClient::ShouldEnableSiteIsolationPolicy() const {
  return true;
}

blink::WebWorkerPermissionClientProxy*
ContentRendererClient::CreateWorkerPermissionClientProxy(
    RenderFrame* render_frame, blink::WebFrame* frame) {
  return NULL;
}

bool ContentRendererClient::IsPluginAllowedToUseCompositorAPI(const GURL& url) {
  return false;
}

bool ContentRendererClient::IsPluginAllowedToUseVideoDecodeAPI(
    const GURL& url) {
  return false;
}

bool ContentRendererClient::IsPluginAllowedToUseDevChannelAPIs() {
  return false;
}

}  // namespace content
