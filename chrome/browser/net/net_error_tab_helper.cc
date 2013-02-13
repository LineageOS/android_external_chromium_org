// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/net_error_tab_helper.h"

#include "base/bind.h"
#include "base/metrics/field_trial.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/net/dns_probe_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/net/net_error_info.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/render_messages.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"

using base::FieldTrialList;
using chrome_common_net::DnsProbeResult;
using content::BrowserContext;
using content::BrowserThread;
using content::PageTransition;
using content::RenderViewHost;
using content::WebContents;
using content::WebContentsObserver;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(chrome_browser_net::NetErrorTabHelper);

namespace chrome_browser_net {

namespace {

const char kDnsProbeFieldTrialName[] = "DnsProbe-Enable";
const char kDnsProbeFieldTrialEnableGroupName[] = "enable";

static NetErrorTabHelper::TestingState testing_state_ =
    NetErrorTabHelper::TESTING_DEFAULT;

// Returns whether |net_error| is a DNS-related error (and therefore whether
// the tab helper should start a DNS probe after receiving it.)
bool IsDnsError(int net_error) {
  return net_error == net::ERR_NAME_NOT_RESOLVED ||
         net_error == net::ERR_NAME_RESOLUTION_FAILED;
}

bool GetEnabledByTrial() {
  return (FieldTrialList::FindFullName(kDnsProbeFieldTrialName)
          == kDnsProbeFieldTrialEnableGroupName);
}

void OnDnsProbeFinishedOnIOThread(
    const base::Callback<void(DnsProbeResult)>& callback,
    DnsProbeResult result) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(callback, result));
}

void StartDnsProbeOnIOThread(
    const base::Callback<void(DnsProbeResult)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  DnsProbeService* probe_service =
      g_browser_process->io_thread()->globals()->dns_probe_service.get();

  probe_service->ProbeDns(base::Bind(&OnDnsProbeFinishedOnIOThread, callback));
}

}  // namespace

NetErrorTabHelper::~NetErrorTabHelper() {
}

// static
void NetErrorTabHelper::set_state_for_testing(TestingState state) {
  testing_state_ = state;
}

void NetErrorTabHelper::DidStartProvisionalLoadForFrame(
    int64 frame_id,
    int64 parent_frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    bool is_error_page,
    bool is_iframe_srcdoc,
    RenderViewHost* render_view_host) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_main_frame)
    return;

  if (is_error_page) {
    error_page_state_ = ERROR_PAGE_STARTED;
  } else {
    error_page_state_ = ERROR_PAGE_NONE;
  }
}

void NetErrorTabHelper::DidCommitProvisionalLoadForFrame(
    int64 frame_id,
    bool is_main_frame,
    const GURL& url,
    PageTransition transition_type,
    RenderViewHost* render_view_host) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_main_frame)
    return;

  if (error_page_state_ == ERROR_PAGE_STARTED) {
    error_page_state_ = ERROR_PAGE_COMMITTED;
  }
}

void NetErrorTabHelper::DidFailProvisionalLoad(
    int64 frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    int error_code,
    const string16& error_description,
    RenderViewHost* render_view_host) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_main_frame)
    return;

  // Consider running a DNS probe if a main frame load fails with a DNS error
  if (error_page_state_ == ERROR_PAGE_STARTED) {
    error_page_state_ = ERROR_PAGE_NONE;
  } else if (IsDnsError(error_code)) {
    OnMainFrameDnsError();
  }
}

void NetErrorTabHelper::DidFinishLoad(
    int64 frame_id,
    const GURL& validated_url,
    bool is_main_frame,
    RenderViewHost* render_view_host) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_main_frame)
    return;

  if (error_page_state_ == ERROR_PAGE_COMMITTED) {
    error_page_state_ = ERROR_PAGE_LOADED;
    MaybeSendInfo();
  }
}

NetErrorTabHelper::NetErrorTabHelper(WebContents* contents)
    : WebContentsObserver(contents),
      dns_probe_state_(DNS_PROBE_NONE),
      error_page_state_(ERROR_PAGE_NONE),
      enabled_by_trial_(GetEnabledByTrial()),
      pref_initialized_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  InitializePref(contents);
}

void NetErrorTabHelper::OnDnsProbeFinishedForTesting(DnsProbeResult result) {
  OnDnsProbeFinished(result);
}

void NetErrorTabHelper::OnMainFrameDnsError() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  bool probe_running = (dns_probe_state_ == DNS_PROBE_STARTED);
  if (probe_running || !ProbesAllowed())
    return;

  PostStartDnsProbeTask();

  dns_probe_state_ = DNS_PROBE_STARTED;
}

void NetErrorTabHelper::PostStartDnsProbeTask() {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&StartDnsProbeOnIOThread,
                 base::Bind(&NetErrorTabHelper::OnDnsProbeFinished,
                            weak_factory_.GetWeakPtr())));
}

void NetErrorTabHelper::OnDnsProbeFinished(DnsProbeResult result) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK_EQ(DNS_PROBE_STARTED, dns_probe_state_);

  dns_probe_result_ = result;
  dns_probe_state_ = DNS_PROBE_FINISHED;
  MaybeSendInfo();
}

void NetErrorTabHelper::MaybeSendInfo() {
  if (dns_probe_state_ != DNS_PROBE_FINISHED
      || error_page_state_ != ERROR_PAGE_LOADED) {
    return;
  }

  SendInfo();

  dns_probe_state_ = DNS_PROBE_NONE;
  error_page_state_ = ERROR_PAGE_NONE;
}

void NetErrorTabHelper::SendInfo() {
  Send(new ChromeViewMsg_NetErrorInfo(routing_id(), dns_probe_result_));
}

void NetErrorTabHelper::InitializePref(WebContents* contents) {
  // Unit tests don't pass a WebContents, so the tab helper has no way to get
  // to the preference.  pref_initialized_ will remain false, so ProbesAllowed
  // will return false without checking the pref.
  if (!contents)
    return;

  BrowserContext* browser_context = contents->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  resolve_errors_with_web_service_.Init(
      prefs::kAlternateErrorPagesEnabled,
      profile->GetPrefs());
  pref_initialized_ = true;
}

bool NetErrorTabHelper::ProbesAllowed() const {
  if (testing_state_ != TESTING_DEFAULT)
    return testing_state_ == TESTING_FORCE_ENABLED;

  // TODO(ttuttle): Disable on mobile?
  return enabled_by_trial_
         && pref_initialized_
         && *resolve_errors_with_web_service_;
}

}  // namespace chrome_browser_net
