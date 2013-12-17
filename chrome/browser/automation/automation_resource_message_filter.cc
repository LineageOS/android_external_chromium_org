// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/automation_resource_message_filter.h"

#include "base/bind.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "chrome/browser/automation/url_request_automation_job.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/common/automation_messages.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/render_messages.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_filter.h"
#include "url/gurl.h"

using content::BrowserMessageFilter;
using content::BrowserThread;

base::LazyInstance<AutomationResourceMessageFilter::RenderViewMap>
    AutomationResourceMessageFilter::filtered_render_views_ =
        LAZY_INSTANCE_INITIALIZER;

int AutomationResourceMessageFilter::unique_request_id_ = 1;
int AutomationResourceMessageFilter::next_completion_callback_id_ = 0;

AutomationResourceMessageFilter::AutomationDetails::AutomationDetails()
    : tab_handle(0),
      ref_count(1),
      is_pending_render_view(false) {
}

AutomationResourceMessageFilter::AutomationDetails::AutomationDetails(
    int tab,
    AutomationResourceMessageFilter* flt,
    bool pending_view)
    : tab_handle(tab), ref_count(1), filter(flt),
      is_pending_render_view(pending_view) {
}

AutomationResourceMessageFilter::AutomationDetails::~AutomationDetails() {}

AutomationResourceMessageFilter::AutomationResourceMessageFilter()
    : channel_(NULL) {
  // Ensure that an instance of the render view map is created.
  filtered_render_views_.Get();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&URLRequestAutomationJob::EnsureProtocolFactoryRegistered));
}

AutomationResourceMessageFilter::~AutomationResourceMessageFilter() {
}

// Called on the IPC thread:
void AutomationResourceMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  DCHECK(!channel_);
  channel_ = channel;
}

void AutomationResourceMessageFilter::OnFilterRemoved() {
  channel_ = NULL;
}

// Called on the IPC thread:
void AutomationResourceMessageFilter::OnChannelConnected(int32 peer_pid) {
}

// Called on the IPC thread:
void AutomationResourceMessageFilter::OnChannelClosing() {
  channel_ = NULL;
  request_map_.clear();

  // Only erase RenderViews which are associated with this
  // AutomationResourceMessageFilter instance.
  RenderViewMap::iterator index = filtered_render_views_.Get().begin();
  while (index != filtered_render_views_.Get().end()) {
    const AutomationDetails& details = (*index).second;
    if (details.filter.get() == this) {
      filtered_render_views_.Get().erase(index++);
    } else {
      index++;
    }
  }
}

// Called on the IPC thread:
bool AutomationResourceMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  int request_id;
  if (URLRequestAutomationJob::MayFilterMessage(message, &request_id)) {
    RequestMap::iterator it = request_map_.find(request_id);
    if (it != request_map_.end()) {
      URLRequestAutomationJob* job = it->second;
      DCHECK(job);
      if (job) {
        job->OnMessage(message);
        return true;
      }
    } else {
      // This could occur if the request was stopped from Chrome which would
      // delete it from the request map. If we receive data for this request
      // from the host we should ignore it.
      LOG(ERROR) << "Failed to find request id:" << request_id;
      return true;
    }
  }

  return false;
}

// Called on the IPC thread:
bool AutomationResourceMessageFilter::Send(IPC::Message* message) {
  // This has to be called on the IO thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!channel_) {
    delete message;
    return false;
  }

  return channel_->Send(message);
}

bool AutomationResourceMessageFilter::RegisterRequest(
    URLRequestAutomationJob* job) {
  if (!job) {
    NOTREACHED();
    return false;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Register pending jobs in the pending request map for servicing later.
  if (job->is_pending()) {
    DCHECK(!ContainsKey(pending_request_map_, job->id()));
    DCHECK(!ContainsKey(request_map_, job->id()));
    pending_request_map_[job->id()] = job;
  } else {
    DCHECK(!ContainsKey(request_map_, job->id()));
    DCHECK(!ContainsKey(pending_request_map_, job->id()));
    request_map_[job->id()] = job;
  }

  return true;
}

void AutomationResourceMessageFilter::UnRegisterRequest(
    URLRequestAutomationJob* job) {
  if (!job) {
    NOTREACHED();
    return;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (job->is_pending()) {
    DCHECK(ContainsKey(pending_request_map_, job->id()));
    pending_request_map_.erase(job->id());
  } else {
    request_map_.erase(job->id());
  }
}

bool AutomationResourceMessageFilter::RegisterRenderView(
    int renderer_pid, int renderer_id, int tab_handle,
    AutomationResourceMessageFilter* filter,
    bool pending_view) {
  if (!renderer_pid || !renderer_id || !tab_handle) {
    NOTREACHED();
    return false;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AutomationResourceMessageFilter::RegisterRenderViewInIOThread,
                 renderer_pid, renderer_id, tab_handle,
                 make_scoped_refptr(filter), pending_view));
  return true;
}

void AutomationResourceMessageFilter::UnRegisterRenderView(
    int renderer_pid, int renderer_id) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &AutomationResourceMessageFilter::UnRegisterRenderViewInIOThread,
          renderer_pid, renderer_id));
}

bool AutomationResourceMessageFilter::ResumePendingRenderView(
    int renderer_pid, int renderer_id, int tab_handle,
    AutomationResourceMessageFilter* filter) {
  if (!renderer_pid || !renderer_id || !tab_handle) {
    NOTREACHED();
    return false;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &AutomationResourceMessageFilter::ResumePendingRenderViewInIOThread,
          renderer_pid, renderer_id, tab_handle, make_scoped_refptr(filter)));
  return true;
}

void AutomationResourceMessageFilter::RegisterRenderViewInIOThread(
    int renderer_pid, int renderer_id,
    int tab_handle, AutomationResourceMessageFilter* filter,
    bool pending_view) {
  RendererId renderer_key(renderer_pid, renderer_id);

  RenderViewMap::iterator automation_details_iter(
      filtered_render_views_.Get().find(renderer_key));
  // We need to match the renderer key and the AutomationResourceMessageFilter
  // instances. If the filter instances are different it means that a new
  // automation channel (External host process) was created for this tab.
  if (automation_details_iter != filtered_render_views_.Get().end() &&
      automation_details_iter->second.filter.get() == filter) {
    DCHECK_GT(automation_details_iter->second.ref_count, 0);
    automation_details_iter->second.ref_count++;
    // The tab handle and the pending status may have changed:-
    // 1.A external tab container is being destroyed and a new one is being
    //   created.
    // 2.The external tab container being destroyed receives a RVH created
    //   notification for the new RVH created to host the newly created tab.
    //   In this case the tab handle in the AutomationDetails structure would
    //   be invalid as it points to a destroyed tab.
    // We need to replace the handle of the external tab being destroyed with
    // the new one that is being created."
    automation_details_iter->second.tab_handle = tab_handle;
    automation_details_iter->second.is_pending_render_view = pending_view;
  } else {
    filtered_render_views_.Get()[renderer_key] =
        AutomationDetails(tab_handle, filter, pending_view);
  }
}

// static
void AutomationResourceMessageFilter::UnRegisterRenderViewInIOThread(
    int renderer_pid, int renderer_id) {
  RenderViewMap::iterator automation_details_iter(
      filtered_render_views_.Get().find(RendererId(renderer_pid,
                                                   renderer_id)));

  if (automation_details_iter == filtered_render_views_.Get().end()) {
    // This is called for all RenderViewHosts, so it's fine if we don't find a
    // match.
    return;
  }

  automation_details_iter->second.ref_count--;

  if (automation_details_iter->second.ref_count <= 0) {
    filtered_render_views_.Get().erase(RendererId(renderer_pid, renderer_id));
  }
}

// static
void AutomationResourceMessageFilter::ResumePendingRenderViewInIOThread(
    int renderer_pid, int renderer_id, int tab_handle,
    AutomationResourceMessageFilter* filter) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  RendererId renderer_key(renderer_pid, renderer_id);

  RenderViewMap::iterator automation_details_iter(
      filtered_render_views_.Get().find(renderer_key));

  DCHECK(automation_details_iter != filtered_render_views_.Get().end())
      << "Failed to find pending view for renderer pid:"
      << renderer_pid << ", render view id:" << renderer_id;

  DCHECK(automation_details_iter->second.is_pending_render_view);

  AutomationResourceMessageFilter* old_filter =
      automation_details_iter->second.filter.get();
  DCHECK(old_filter != NULL);

  filtered_render_views_.Get()[renderer_key] =
      AutomationDetails(tab_handle, filter, false);

  ResumeJobsForPendingView(tab_handle, old_filter, filter);
}

bool AutomationResourceMessageFilter::LookupRegisteredRenderView(
    int renderer_pid, int renderer_id, AutomationDetails* details) {
  bool found = false;
  RenderViewMap::iterator it = filtered_render_views_.Get().find(RendererId(
      renderer_pid, renderer_id));
  if (it != filtered_render_views_.Get().end()) {
    found = true;
    if (details)
      *details = it->second;
  }

  return found;
}

bool AutomationResourceMessageFilter::GetAutomationRequestId(
    int request_id, int* automation_request_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  RequestMap::iterator it = request_map_.begin();
  while (it != request_map_.end()) {
    URLRequestAutomationJob* job = it->second;
    DCHECK(job);
    if (job && job->request_id() == request_id) {
      *automation_request_id = job->id();
      return true;
    }
    it++;
  }

  return false;
}

// static
void AutomationResourceMessageFilter::ResumeJobsForPendingView(
    int tab_handle,
    AutomationResourceMessageFilter* old_filter,
    AutomationResourceMessageFilter* new_filter) {
  DCHECK(old_filter != NULL);
  DCHECK(new_filter != NULL);

  RequestMap pending_requests = old_filter->pending_request_map_;
  old_filter->pending_request_map_.clear();

  for (RequestMap::iterator index = pending_requests.begin();
       index != pending_requests.end(); index++) {
    URLRequestAutomationJob* job = (*index).second;
    DCHECK_EQ(job->message_filter(), old_filter);
    DCHECK(job->is_pending());
    // StartPendingJob will register the job with the new filter.
    job->StartPendingJob(tab_handle, new_filter);
  }
}
