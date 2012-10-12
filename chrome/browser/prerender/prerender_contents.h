// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "base/values.h"
#include "chrome/browser/prerender/prerender_final_status.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/referrer.h"
#include "ui/gfx/size.h"

class Profile;
class TabContents;
struct FaviconURL;

namespace base {
class ProcessMetrics;
}

namespace content {
class RenderViewHost;
class SessionStorageNamespace;
class WebContents;
}

namespace history {
struct HistoryAddPageArgs;
}

namespace prerender {

class PrerenderHandle;
class PrerenderManager;
class PrerenderRenderViewHostObserver;
class PrerenderTracker;

class PrerenderContents : public content::NotificationObserver,
                          public content::WebContentsObserver {
 public:
  // PrerenderContents::Create uses the currently registered Factory to create
  // the PrerenderContents. Factory is intended for testing.
  class Factory {
   public:
    Factory() {}
    virtual ~Factory() {}

    // Ownership is not transfered through this interface as prerender_manager,
    // prerender_tracker, and profile are stored as weak pointers.
    virtual PrerenderContents* CreatePrerenderContents(
        PrerenderManager* prerender_manager,
        PrerenderTracker* prerender_tracker,
        Profile* profile,
        const GURL& url,
        const content::Referrer& referrer,
        Origin origin,
        uint8 experiment_id) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Factory);
  };

  // Indicates how this PrerenderContents relates to MatchComplete. This is to
  // figure out which histograms to use to record the FinalStatus, Match (record
  // all prerenders and control group prerenders) or MatchComplete (record
  // running prerenders only in the way they would have been recorded in the
  // control group).
  enum MatchCompleteStatus {
    // A regular prerender which will be recorded both in Match and
    // MatchComplete.
    MATCH_COMPLETE_DEFAULT,
    // A prerender that used to be a regular prerender, but has since been
    // replaced by a MatchComplete dummy. Therefore, we will record this only
    // for Match, but not for MatchComplete.
    MATCH_COMPLETE_REPLACED,
    // A prerender that is a MatchComplete dummy replacing a regular prerender.
    // In the control group, our prerender never would have been canceled, so
    // we record in MatchComplete but not Match.
    MATCH_COMPLETE_REPLACEMENT,
    // A prerender that is a MatchComplete dummy, early in the process of being
    // created. This prerender should not fail. Record for MatchComplete, but
    // not Match.
    MATCH_COMPLETE_REPLACEMENT_PENDING,
  };

  virtual ~PrerenderContents();

  // For MatchComplete correctness, create a dummy replacement prerender
  // contents to stand in for this prerender contents that (which we are about
  // to destroy).
  void MakeIntoDummyReplacementOf(
      const PrerenderContents* original_prerender_contents);

  bool Init();

  static Factory* CreateFactory();

  // Start rendering the contents in the prerendered state. If
  // |is_control_group| is true, this will go through some of the mechanics of
  // starting a prerender, without actually creating the RenderView.
  // |creator_child_id| is the id of the child process that caused the prerender
  // to be created, and is needed so that the prerendered URLs can be sent to it
  // so render-initiated navigations will swap in the prerendered page. |size|
  // indicates the rectangular dimensions that the prerendered page should be.
  // |session_storage_namespace| indicates the namespace that the prerendered
  // page should be part of.
  virtual void StartPrerendering(
      int creator_child_id,
      const gfx::Size& size,
      content::SessionStorageNamespace* session_storage_namespace,
      bool is_control_group);

  // Verifies that the prerendering is not using too many resources, and kills
  // it if not.
  void DestroyWhenUsingTooManyResources();

  content::RenderViewHost* GetRenderViewHostMutable();
  const content::RenderViewHost* GetRenderViewHost() const;

  string16 title() const { return title_; }
  int32 page_id() const { return page_id_; }
  GURL icon_url() const { return icon_url_; }
  const GURL& prerender_url() const { return prerender_url_; }
  const content::Referrer& referrer() const { return referrer_; }
  bool has_stopped_loading() const { return has_stopped_loading_; }
  bool has_finished_loading() const { return has_finished_loading_; }
  bool prerendering_has_started() const { return prerendering_has_started_; }
  MatchCompleteStatus match_complete_status() const {
    return match_complete_status_;
  }
  void set_match_complete_status(MatchCompleteStatus status) {
    match_complete_status_ = status;
  }

  // Sets the parameter to the value of the associated RenderViewHost's child id
  // and returns a boolean indicating the validity of that id.
  virtual bool GetChildId(int* child_id) const;

  // Sets the parameter to the value of the associated RenderViewHost's route id
  // and returns a boolean indicating the validity of that id.
  virtual bool GetRouteId(int* route_id) const;

  // Set the final status for how the PrerenderContents was used. This
  // should only be called once, and should be called before the prerender
  // contents are destroyed.
  void set_final_status(FinalStatus final_status);
  FinalStatus final_status() const { return final_status_; }

  Origin origin() const { return origin_; }
  uint8 experiment_id() const { return experiment_id_; }

  base::TimeTicks load_start_time() const { return load_start_time_; }

  // Indicates whether this prerendered page can be used for the provided
  // |url| and |session_storage_namespace|.
  bool Matches(
      const GURL& url,
      const content::SessionStorageNamespace* session_storage_namespace) const;

  // content::WebContentsObserver implementation.
  virtual void DidStopLoading(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidStartProvisionalLoadForFrame(
      int64 frame_id,
      int64 parent_frame_id,
      bool is_main_frame,
      const GURL& validated_url,
      bool is_error_page,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidFinishLoad(
      int64 frame_id,
      const GURL& validated_url,
      bool is_main_frame,
      content::RenderViewHost* render_view_host) OVERRIDE;

  virtual void RenderViewGone(base::TerminationStatus status) OVERRIDE;

  // content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Adds an alias URL, for one of the many redirections. If the URL can not
  // be prerendered - for example, it's an ftp URL - |this| will be destroyed
  // and false is returned. Otherwise, true is returned and the alias is
  // remembered.
  virtual bool AddAliasURL(const GURL& url);

  // Adds all alias URLs from another prerender.
  void AddAliasURLsFromOtherPrerenderContents(PrerenderContents* other_pc);

  // The preview TabContents (may be null).
  TabContents* prerender_contents() const {
    return prerender_contents_.get();
  }

  TabContents* ReleasePrerenderContents();

  // Sets the final status, calls OnDestroy and adds |this| to the
  // PrerenderManager's pending deletes list.
  void Destroy(FinalStatus reason);

  // Called by the history tab helper with the information that it woudl have
  // added to the history service had this web contents not been used for
  // prerendering.
  void DidNavigate(const history::HistoryAddPageArgs& add_page_args);

  // Applies all the URL history encountered during prerendering to the
  // new tab.
  void CommitHistory(TabContents* tab);

  base::Value* GetAsValue() const;

  // Returns whether a pending cross-site navigation is happening.
  // This could happen with renderer-issued navigations, such as a
  // MouseEvent being dispatched by a link to a website installed as an app.
  bool IsCrossSiteNavigationPending() const;

  // Adds a pending prerender to the list. If |weak_prerender_handle| still
  // exists when this page is made visible, it will be launched.
  virtual void AddPendingPrerender(
      base::WeakPtr<PrerenderHandle> weak_prerender_handle,
      const GURL& url,
      const content::Referrer& referrer,
      const gfx::Size& size);

  // Returns true if |url| corresponds to a pending prerender.
  bool IsPendingEntry(const PrerenderHandle& prerender_handle) const;

  // Reissues any pending prerender requests from the prerendered page.  Also
  // clears the list of pending requests.
  void StartPendingPrerenders();

 protected:
  // Information on pages that the prerendered page has tried to prerender.
  struct PendingPrerenderInfo {
    PendingPrerenderInfo(
        base::WeakPtr<PrerenderHandle> weak_prerender_handle,
        const GURL& url,
        const content::Referrer& referrer,
        const gfx::Size& size);
    ~PendingPrerenderInfo();

    base::WeakPtr<PrerenderHandle> weak_prerender_handle;
    GURL url;
    content::Referrer referrer;
    gfx::Size size;
  };

  PrerenderContents(PrerenderManager* prerender_manager,
                    PrerenderTracker* prerender_tracker,
                    Profile* profile,
                    const GURL& url,
                    const content::Referrer& referrer,
                    Origin origin,
                    uint8 experiment_id);

  // Called whenever a RenderViewHost is created for prerendering.  Only called
  // once the RenderViewHost has a RenderView and RenderWidgetHostView.
  virtual void OnRenderViewHostCreated(
      content::RenderViewHost* new_render_view_host);

  content::NotificationRegistrar& notification_registrar() {
    return notification_registrar_;
  }

  const std::vector<PendingPrerenderInfo>& pending_prerenders() const {
    return pending_prerenders_;
  }

  bool prerendering_has_been_cancelled() const {
    return prerendering_has_been_cancelled_;
  }

  virtual content::WebContents* CreateWebContents(
      content::SessionStorageNamespace* session_storage_namespace);

  bool prerendering_has_started_;

  // Time at which we started to load the URL.  This is used to compute
  // the time elapsed from initiating a prerender until the time the
  // (potentially only partially) prerendered page is shown to the user.
  base::TimeTicks load_start_time_;

 private:
  class TabContentsDelegateImpl;

  // Needs to be able to call the constructor.
  friend class PrerenderContentsFactoryImpl;

  friend class PrerenderRenderViewHostObserver;

  // Message handlers.
  void OnUpdateFaviconURL(int32 page_id, const std::vector<FaviconURL>& urls);

  // Returns the RenderViewHost Delegate for this prerender.
  content::WebContents* GetWebContents();

  // Returns the ProcessMetrics for the render process, if it exists.
  base::ProcessMetrics* MaybeGetProcessMetrics();

  // The prerender manager owning this object.
  PrerenderManager* prerender_manager_;

  // The prerender tracker tracking prerenders.
  PrerenderTracker* prerender_tracker_;

  // The URL being prerendered.
  GURL prerender_url_;

  // The referrer.
  content::Referrer referrer_;

  // The profile being used
  Profile* profile_;

  // Information about the title and URL of the page that this class as a
  // RenderViewHostDelegate has received from the RenderView.
  // Used to apply to the new RenderViewHost delegate that might eventually
  // own the contained RenderViewHost when the prerendered page is shown
  // in a WebContents.
  string16 title_;
  int32 page_id_;
  GURL url_;
  GURL icon_url_;
  content::NotificationRegistrar notification_registrar_;

  // A vector of URLs that this prerendered page matches against.
  // This array can contain more than element as a result of redirects,
  // such as HTTP redirects or javascript redirects.
  std::vector<GURL> alias_urls_;

  // The session storage namespace id for use in matching. We must save it
  // rather than get it from the RenderViewHost since in the control group
  // we won't have a RenderViewHost.
  int64 session_storage_namespace_id_;

  bool has_stopped_loading_;

  // True when the main frame has finished loading.
  bool has_finished_loading_;

  // This must be the same value as the PrerenderTracker has recorded for
  // |this|, when |this| has a RenderView.
  FinalStatus final_status_;

  // The MatchComplete status of the prerender, indicating how it relates
  // to being a MatchComplete dummy (see definition of MatchCompleteStatus
  // above).
  MatchCompleteStatus match_complete_status_;

  // Tracks whether or not prerendering has been cancelled by calling Destroy.
  // Used solely to prevent double deletion.
  bool prerendering_has_been_cancelled_;

  // Process Metrics of the render process associated with the
  // RenderViewHost for this object.
  scoped_ptr<base::ProcessMetrics> process_metrics_;

  // The prerendered TabContents; may be null.
  scoped_ptr<TabContents> prerender_contents_;

  scoped_ptr<PrerenderRenderViewHostObserver> render_view_host_observer_;

  scoped_ptr<TabContentsDelegateImpl> tab_contents_delegate_;

  // These are -1 before a RenderView is created.
  int child_id_;
  int route_id_;

  // Origin for this prerender.
  Origin origin_;

  // Experiment during which this prerender is performed.
  uint8 experiment_id_;

  // List of all pages the prerendered page has tried to prerender.
  std::vector<PendingPrerenderInfo> pending_prerenders_;

  // The process that created the child id.
  int creator_child_id_;

  // The size of the WebView from the launching page.
  gfx::Size size_;

  typedef std::vector<history::HistoryAddPageArgs> AddPageVector;

  // Caches pages to be added to the history.
  AddPageVector add_page_vector_;

  DISALLOW_COPY_AND_ASSIGN(PrerenderContents);
};

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_
