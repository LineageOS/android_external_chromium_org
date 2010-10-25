// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "app/l10n_util.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/sys_info.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/app_modal_dialog.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_init.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/js_modal_dialog.h"
#include "chrome/browser/native_app_modal_dialog.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tabs/pinned_tab_codec.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/page_transition_types.h"
#include "chrome/test/in_process_browser_test.h"
#include "chrome/test/ui_test_utils.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "net/base/mock_host_resolver.h"
#include "net/test/test_server.h"

#if defined(OS_WIN)
#include "base/i18n/rtl.h"
#endif

namespace {

const std::string BEFORE_UNLOAD_HTML =
    "<html><head><title>beforeunload</title></head><body>"
    "<script>window.onbeforeunload=function(e){return 'foo'}</script>"
    "</body></html>";

const std::wstring OPEN_NEW_BEFOREUNLOAD_PAGE =
    L"w=window.open(); w.onbeforeunload=function(e){return 'foo'};";

const FilePath::CharType* kTitle1File = FILE_PATH_LITERAL("title1.html");
const FilePath::CharType* kTitle2File = FILE_PATH_LITERAL("title2.html");

const FilePath::CharType kDocRoot[] = FILE_PATH_LITERAL("chrome/test/data");

// Given a page title, returns the expected window caption string.
std::wstring WindowCaptionFromPageTitle(std::wstring page_title) {
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
  // On Mac or ChromeOS, we don't want to suffix the page title with
  // the application name.
  if (page_title.empty())
    return l10n_util::GetString(IDS_BROWSER_WINDOW_MAC_TAB_UNTITLED);
  return page_title;
#else
  if (page_title.empty())
    return l10n_util::GetString(IDS_PRODUCT_NAME);

  return l10n_util::GetStringF(IDS_BROWSER_WINDOW_TITLE_FORMAT, page_title);
#endif
}

// Returns the number of active RenderProcessHosts.
int CountRenderProcessHosts() {
  int result = 0;
  for (RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance())
    ++result;
  return result;
}

class MockTabStripModelObserver : public TabStripModelObserver {
 public:
  MockTabStripModelObserver() : closing_count_(0) {}

  virtual void TabClosingAt(TabStripModel* tab_strip_model,
                            TabContents* contents,
                            int index) {
    closing_count_++;
  }

  int closing_count() const { return closing_count_; }

 private:
  int closing_count_;

  DISALLOW_COPY_AND_ASSIGN(MockTabStripModelObserver);
};

// Used by CloseWithAppMenuOpen. Invokes CloseWindow on the supplied browser.
class CloseWindowTask : public Task {
 public:
  explicit CloseWindowTask(Browser* browser) : browser_(browser) {}

  virtual void Run() {
    browser_->CloseWindow();
  }

 private:
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(CloseWindowTask);
};

// Used by CloseWithAppMenuOpen. Posts a CloseWindowTask and shows the app menu.
class RunCloseWithAppMenuTask : public Task {
 public:
  explicit RunCloseWithAppMenuTask(Browser* browser) : browser_(browser) {}

  virtual void Run() {
    // ShowAppMenu is modal under views. Schedule a task that closes the window.
    MessageLoop::current()->PostTask(FROM_HERE, new CloseWindowTask(browser_));
    browser_->ShowAppMenu();
  }

 private:
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(RunCloseWithAppMenuTask);
};

}  // namespace

class BrowserTest : public ExtensionBrowserTest {
 protected:
  // In RTL locales wrap the page title with RTL embedding characters so that it
  // matches the value returned by GetWindowTitle().
  std::wstring LocaleWindowCaptionFromPageTitle(
      const std::wstring& expected_title) {
    std::wstring page_title = WindowCaptionFromPageTitle(expected_title);
#if defined(OS_WIN)
    std::string locale = g_browser_process->GetApplicationLocale();
    if (base::i18n::GetTextDirectionForLocale(locale.c_str()) ==
        base::i18n::RIGHT_TO_LEFT) {
      base::i18n::WrapStringWithLTRFormatting(&page_title);
    }

    return page_title;
#else
    // Do we need to use the above code on POSIX as well?
    return page_title;
#endif
  }

  // Returns the app extension aptly named "App Test".
  Extension* GetExtension() {
    const ExtensionList* extensions =
        browser()->profile()->GetExtensionsService()->extensions();
    for (size_t i = 0; i < extensions->size(); ++i) {
      if ((*extensions)[i]->name() == "App Test")
        return (*extensions)[i];
    }
    NOTREACHED();
    return NULL;
  }
};

// Launch the app on a page with no title, check that the app title was set
// correctly.
IN_PROC_BROWSER_TEST_F(BrowserTest, NoTitle) {
  ui_test_utils::NavigateToURL(browser(),
      ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                FilePath(kTitle1File)));
  EXPECT_EQ(LocaleWindowCaptionFromPageTitle(L"title1.html"),
            UTF16ToWideHack(browser()->GetWindowTitleForCurrentTab()));
  string16 tab_title;
  ASSERT_TRUE(ui_test_utils::GetCurrentTabTitle(browser(), &tab_title));
  EXPECT_EQ(ASCIIToUTF16("title1.html"), tab_title);
}

// Launch the app, navigate to a page with a title, check that the app title
// was set correctly.
IN_PROC_BROWSER_TEST_F(BrowserTest, Title) {
  ui_test_utils::NavigateToURL(browser(),
      ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                FilePath(kTitle2File)));
  const std::wstring test_title(L"Title Of Awesomeness");
  EXPECT_EQ(LocaleWindowCaptionFromPageTitle(test_title),
            UTF16ToWideHack(browser()->GetWindowTitleForCurrentTab()));
  string16 tab_title;
  ASSERT_TRUE(ui_test_utils::GetCurrentTabTitle(browser(), &tab_title));
  EXPECT_EQ(WideToUTF16(test_title), tab_title);
}

#if defined(OS_MACOSX)
// Test is crashing on Mac, see http://crbug.com/29424.
#define MAYBE_JavascriptAlertActivatesTab DISABLED_JavascriptAlertActivatesTab
#else
#define MAYBE_JavascriptAlertActivatesTab JavascriptAlertActivatesTab
#endif

IN_PROC_BROWSER_TEST_F(BrowserTest, MAYBE_JavascriptAlertActivatesTab) {
  GURL url(ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                     FilePath(kTitle1File)));
  ui_test_utils::NavigateToURL(browser(), url);
  Browser::AddTabWithURLParams params(url, PageTransition::TYPED);
  params.index = 0;
  browser()->AddTabWithURL(&params);
  EXPECT_EQ(browser(), params.target);
  EXPECT_EQ(2, browser()->tab_count());
  EXPECT_EQ(0, browser()->selected_index());
  TabContents* second_tab = browser()->GetTabContentsAt(1);
  ASSERT_TRUE(second_tab);
  second_tab->render_view_host()->ExecuteJavascriptInWebFrame(L"",
      L"alert('Activate!');");
  AppModalDialog* alert = ui_test_utils::WaitForAppModalDialog();
  alert->CloseModalDialog();
  EXPECT_EQ(2, browser()->tab_count());
  EXPECT_EQ(1, browser()->selected_index());
}

// Create 34 tabs and verify that a lot of processes have been created. The
// exact number of processes depends on the amount of memory. Previously we
// had a hard limit of 31 processes and this test is mainly directed at
// verifying that we don't crash when we pass this limit.
// Warning: this test can take >30 seconds when running on a slow (low
// memory?) Mac builder.
IN_PROC_BROWSER_TEST_F(BrowserTest, ThirtyFourTabs) {
  GURL url(ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                     FilePath(kTitle2File)));

  // There is one initial tab.
  for (int ix = 0; ix != 33; ++ix) {
    Browser::AddTabWithURLParams params(url, PageTransition::TYPED);
    params.index = 0;
    browser()->AddTabWithURL(&params);
    EXPECT_EQ(browser(), params.target);
  }
  EXPECT_EQ(34, browser()->tab_count());

  // See browser\renderer_host\render_process_host.cc for the algorithm to
  // decide how many processes to create.
  if (base::SysInfo::AmountOfPhysicalMemoryMB() >= 2048) {
    EXPECT_GE(CountRenderProcessHosts(), 24);
  } else {
    EXPECT_LE(CountRenderProcessHosts(), 23);
  }
}

// Test for crbug.com/22004.  Reloading a page with a before unload handler and
// then canceling the dialog should not leave the throbber spinning.
IN_PROC_BROWSER_TEST_F(BrowserTest, ReloadThenCancelBeforeUnload) {
  GURL url("data:text/html," + BEFORE_UNLOAD_HTML);
  ui_test_utils::NavigateToURL(browser(), url);

  // Navigate to another page, but click cancel in the dialog.  Make sure that
  // the throbber stops spinning.
  browser()->Reload(CURRENT_TAB);
  AppModalDialog* alert = ui_test_utils::WaitForAppModalDialog();
  alert->CloseModalDialog();
  EXPECT_FALSE(browser()->GetSelectedTabContents()->is_loading());

  // Clear the beforeunload handler so the test can easily exit.
  browser()->GetSelectedTabContents()->render_view_host()->
      ExecuteJavascriptInWebFrame(L"", L"onbeforeunload=null;");
}

// Crashy on mac.  http://crbug.com/38522
#if defined(OS_MACOSX)
#define MAYBE_SingleBeforeUnloadAfterWindowClose \
        DISABLED_SingleBeforeUnloadAfterWindowClose
#else
#define MAYBE_SingleBeforeUnloadAfterWindowClose \
        SingleBeforeUnloadAfterWindowClose
#endif

// Test for crbug.com/11647.  A page closed with window.close() should not have
// two beforeunload dialogs shown.
IN_PROC_BROWSER_TEST_F(BrowserTest, MAYBE_SingleBeforeUnloadAfterWindowClose) {
  browser()->GetSelectedTabContents()->render_view_host()->
      ExecuteJavascriptInWebFrame(L"", OPEN_NEW_BEFOREUNLOAD_PAGE);

  // Close the new window with JavaScript, which should show a single
  // beforeunload dialog.  Then show another alert, to make it easy to verify
  // that a second beforeunload dialog isn't shown.
  browser()->GetTabContentsAt(0)->render_view_host()->
      ExecuteJavascriptInWebFrame(L"", L"w.close(); alert('bar');");
  AppModalDialog* alert = ui_test_utils::WaitForAppModalDialog();
  alert->native_dialog()->AcceptAppModalDialog();

  alert = ui_test_utils::WaitForAppModalDialog();
  EXPECT_FALSE(static_cast<JavaScriptAppModalDialog*>(alert)->
                   is_before_unload_dialog());
  alert->native_dialog()->AcceptAppModalDialog();
}

// Test that get_process_idle_time() returns reasonable values when compared
// with time deltas measured locally.
IN_PROC_BROWSER_TEST_F(BrowserTest, RenderIdleTime) {
  base::TimeTicks start = base::TimeTicks::Now();
  ui_test_utils::NavigateToURL(browser(),
      ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                FilePath(kTitle1File)));
  RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
  for (; !it.IsAtEnd(); it.Advance()) {
    base::TimeDelta renderer_td =
        it.GetCurrentValue()->get_child_process_idle_time();
    base::TimeDelta browser_td = base::TimeTicks::Now() - start;
    EXPECT_TRUE(browser_td >= renderer_td);
  }
}

// Test IDC_CREATE_SHORTCUTS command is enabled for url scheme file, ftp, http
// and https and disabled for chrome://, about:// etc.
// TODO(pinkerton): Disable app-mode in the model until we implement it
// on the Mac. http://crbug.com/13148
#if !defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(BrowserTest, CommandCreateAppShortcutFile) {
  CommandUpdater* command_updater = browser()->command_updater();

  static const FilePath::CharType* kEmptyFile = FILE_PATH_LITERAL("empty.html");
  GURL file_url(ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                          FilePath(kEmptyFile)));
  ASSERT_TRUE(file_url.SchemeIs(chrome::kFileScheme));
  ui_test_utils::NavigateToURL(browser(), file_url);
  EXPECT_TRUE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));
}

IN_PROC_BROWSER_TEST_F(BrowserTest, CommandCreateAppShortcutHttp) {
  CommandUpdater* command_updater = browser()->command_updater();

  ASSERT_TRUE(test_server()->Start());
  GURL http_url(test_server()->GetURL(""));
  ASSERT_TRUE(http_url.SchemeIs(chrome::kHttpScheme));
  ui_test_utils::NavigateToURL(browser(), http_url);
  EXPECT_TRUE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));
}

IN_PROC_BROWSER_TEST_F(BrowserTest, CommandCreateAppShortcutHttps) {
  CommandUpdater* command_updater = browser()->command_updater();

  net::TestServer test_server(net::TestServer::TYPE_HTTPS, FilePath(kDocRoot));
  ASSERT_TRUE(test_server.Start());
  GURL https_url(test_server.GetURL("/"));
  ASSERT_TRUE(https_url.SchemeIs(chrome::kHttpsScheme));
  ui_test_utils::NavigateToURL(browser(), https_url);
  EXPECT_TRUE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));
}

IN_PROC_BROWSER_TEST_F(BrowserTest, CommandCreateAppShortcutFtp) {
  CommandUpdater* command_updater = browser()->command_updater();

  net::TestServer test_server(net::TestServer::TYPE_FTP, FilePath(kDocRoot));
  ASSERT_TRUE(test_server.Start());
  GURL ftp_url(test_server.GetURL(""));
  ASSERT_TRUE(ftp_url.SchemeIs(chrome::kFtpScheme));
  ui_test_utils::NavigateToURL(browser(), ftp_url);
  EXPECT_TRUE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));
}

IN_PROC_BROWSER_TEST_F(BrowserTest, CommandCreateAppShortcutInvalid) {
  CommandUpdater* command_updater = browser()->command_updater();

  // Urls that should not have shortcuts.
  GURL new_tab_url(chrome::kChromeUINewTabURL);
  ui_test_utils::NavigateToURL(browser(), new_tab_url);
  EXPECT_FALSE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));

  GURL history_url(chrome::kChromeUIHistoryURL);
  ui_test_utils::NavigateToURL(browser(), history_url);
  EXPECT_FALSE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));

  GURL downloads_url(chrome::kChromeUIDownloadsURL);
  ui_test_utils::NavigateToURL(browser(), downloads_url);
  EXPECT_FALSE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));

  GURL blank_url(chrome::kAboutBlankURL);
  ui_test_utils::NavigateToURL(browser(), blank_url);
  EXPECT_FALSE(command_updater->IsCommandEnabled(IDC_CREATE_SHORTCUTS));
}
#endif  // !defined(OS_MACOSX)

// Test RenderView correctly send back favicon url for web page that redirects
// to an anchor in javascript body.onload handler.
IN_PROC_BROWSER_TEST_F(BrowserTest, FaviconOfOnloadRedirectToAnchorPage) {
  ASSERT_TRUE(test_server()->Start());
  GURL url(test_server()->GetURL("files/onload_redirect_to_anchor.html"));
  GURL expected_favicon_url(test_server()->GetURL("files/test.png"));

  ui_test_utils::NavigateToURL(browser(), url);

  NavigationEntry* entry = browser()->GetSelectedTabContents()->
      controller().GetActiveEntry();
  EXPECT_EQ(expected_favicon_url.spec(), entry->favicon().url().spec());
}

// Test that an icon can be changed from JS.
IN_PROC_BROWSER_TEST_F(BrowserTest, FaviconChange) {
  static const FilePath::CharType* kFile =
      FILE_PATH_LITERAL("onload_change_favicon.html");
  GURL file_url(ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                          FilePath(kFile)));
  ASSERT_TRUE(file_url.SchemeIs(chrome::kFileScheme));
  ui_test_utils::NavigateToURL(browser(), file_url);

  NavigationEntry* entry = browser()->GetSelectedTabContents()->
      controller().GetActiveEntry();
  static const FilePath::CharType* kIcon =
      FILE_PATH_LITERAL("test1.png");
  GURL expected_favicon_url(
      ui_test_utils::GetTestUrl(FilePath(FilePath::kCurrentDirectory),
                                         FilePath(kIcon)));
  EXPECT_EQ(expected_favicon_url.spec(), entry->favicon().url().spec());
}

// Makes sure TabClosing is sent when uninstalling an extension that is an app
// tab.
IN_PROC_BROWSER_TEST_F(BrowserTest, TabClosingWhenRemovingExtension) {
  ASSERT_TRUE(test_server()->Start());
  host_resolver()->AddRule("www.example.com", "127.0.0.1");
  GURL url(test_server()->GetURL("empty.html"));
  TabStripModel* model = browser()->tabstrip_model();

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("app/")));

  Extension* extension_app = GetExtension();

  ui_test_utils::NavigateToURL(browser(), url);

  TabContents* app_contents = new TabContents(browser()->profile(), NULL,
                                              MSG_ROUTING_NONE, NULL, NULL);
  app_contents->SetExtensionApp(extension_app);

  model->AddTabContents(app_contents, 0, 0, TabStripModel::ADD_NONE);
  model->SetTabPinned(0, true);
  ui_test_utils::NavigateToURL(browser(), url);

  MockTabStripModelObserver observer;
  model->AddObserver(&observer);

  // Uninstall the extension and make sure TabClosing is sent.
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  service->UninstallExtension(GetExtension()->id(), false);
  EXPECT_EQ(1, observer.closing_count());

  model->RemoveObserver(&observer);

  // There should only be one tab now.
  ASSERT_EQ(1, browser()->tab_count());
}

#if defined(OS_WIN)
// http://crbug.com/46198. On XP/Vista, the failure rate is 5 ~ 6%.
#define MAYBE_PageLanguageDetection FLAKY_PageLanguageDetection
#else
#define MAYBE_PageLanguageDetection PageLanguageDetection
#endif
// Tests that the CLD (Compact Language Detection) works properly.
IN_PROC_BROWSER_TEST_F(BrowserTest, MAYBE_PageLanguageDetection) {
  ASSERT_TRUE(test_server()->Start());

  TabContents* current_tab = browser()->GetSelectedTabContents();

  // Navigate to a page in English.
  ui_test_utils::WindowedNotificationObserverWithDetails<TabContents,
                                                         std::string>
      en_language_detected_signal(NotificationType::TAB_LANGUAGE_DETERMINED,
                                  current_tab);
  ui_test_utils::NavigateToURL(
      browser(), GURL(test_server()->GetURL("files/english_page.html")));
  EXPECT_TRUE(current_tab->language_state().original_language().empty());
  en_language_detected_signal.Wait();
  std::string lang;
  EXPECT_TRUE(en_language_detected_signal.GetDetailsFor(current_tab, &lang));
  EXPECT_EQ("en", lang);
  EXPECT_EQ("en", current_tab->language_state().original_language());

  // Now navigate to a page in French.
  ui_test_utils::WindowedNotificationObserverWithDetails<TabContents,
                                                         std::string>
      fr_language_detected_signal(NotificationType::TAB_LANGUAGE_DETERMINED,
                                  current_tab);
  ui_test_utils::NavigateToURL(
      browser(), GURL(test_server()->GetURL("files/french_page.html")));
  EXPECT_TRUE(current_tab->language_state().original_language().empty());
  fr_language_detected_signal.Wait();
  lang.clear();
  EXPECT_TRUE(fr_language_detected_signal.GetDetailsFor(current_tab, &lang));
  EXPECT_EQ("fr", lang);
  EXPECT_EQ("fr", current_tab->language_state().original_language());
}

// Chromeos defaults to restoring the last session, so this test isn't
// applicable.
#if !defined(OS_CHROMEOS)
#if defined(OS_MACOSX)
// Crashy, http://crbug.com/38522
#define RestorePinnedTabs DISABLED_RestorePinnedTabs
#endif
// Makes sure pinned tabs are restored correctly on start.
IN_PROC_BROWSER_TEST_F(BrowserTest, RestorePinnedTabs) {
  ASSERT_TRUE(test_server()->Start());

  // Add an pinned app tab.
  host_resolver()->AddRule("www.example.com", "127.0.0.1");
  GURL url(test_server()->GetURL("empty.html"));
  TabStripModel* model = browser()->tabstrip_model();
  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("app/")));
  Extension* extension_app = GetExtension();
  ui_test_utils::NavigateToURL(browser(), url);
  TabContents* app_contents = new TabContents(browser()->profile(), NULL,
                                              MSG_ROUTING_NONE, NULL, NULL);
  app_contents->SetExtensionApp(extension_app);
  model->AddTabContents(app_contents, 0, 0, TabStripModel::ADD_NONE);
  model->SetTabPinned(0, true);
  ui_test_utils::NavigateToURL(browser(), url);

  // Add a non pinned tab.
  browser()->NewTab();

  // Add a pinned non-app tab.
  browser()->NewTab();
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  model->SetTabPinned(2, true);

  // Write out the pinned tabs.
  PinnedTabCodec::WritePinnedTabs(browser()->profile());

  // Simulate launching again.
  CommandLine dummy(CommandLine::NO_PROGRAM);
  BrowserInit::LaunchWithProfile launch(FilePath(), dummy);
  launch.profile_ = browser()->profile();
  launch.ProcessStartupURLs(std::vector<GURL>());

  // The launch should have created a new browser.
  ASSERT_EQ(2u, BrowserList::GetBrowserCount(browser()->profile()));

  // Find the new browser.
  Browser* new_browser = NULL;
  for (BrowserList::const_iterator i = BrowserList::begin();
       i != BrowserList::end() && !new_browser; ++i) {
    if (*i != browser())
      new_browser = *i;
  }
  ASSERT_TRUE(new_browser);
  ASSERT_TRUE(new_browser != browser());

  // We should get back an additional tab for the app.
  ASSERT_EQ(2, new_browser->tab_count());

  // Make sure the state matches.
  TabStripModel* new_model = new_browser->tabstrip_model();
  EXPECT_TRUE(new_model->IsAppTab(0));
  EXPECT_FALSE(new_model->IsAppTab(1));

  EXPECT_TRUE(new_model->IsTabPinned(0));
  EXPECT_TRUE(new_model->IsTabPinned(1));

  EXPECT_TRUE(new_model->GetTabContentsAt(0)->extension_app() ==
              extension_app);
}
#endif  // !defined(OS_CHROMEOS)

// This test verifies we don't crash when closing the last window and the app
// menu is showing.
IN_PROC_BROWSER_TEST_F(BrowserTest, CloseWithAppMenuOpen) {
  if (browser_defaults::kBrowserAliveWithNoWindows)
    return;

  // We need a message loop running for menus on windows.
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new RunCloseWithAppMenuTask(browser()));
}

// TODO(ben): this test was never enabled. It has bit-rotted since being added.
// It originally lived in browser_unittest.cc, but has been moved here to make
// room for real browser unit tests.
#if 0
class BrowserTest2 : public InProcessBrowserTest {
 public:
  BrowserTest2() {
    host_resolver_proc_ = new net::RuleBasedHostResolverProc(NULL);
    // Avoid making external DNS lookups. In this test we don't need this
    // to succeed.
    host_resolver_proc_->AddSimulatedFailure("*.google.com");
    scoped_host_resolver_proc_.Init(host_resolver_proc_.get());
  }

 private:
  scoped_refptr<net::RuleBasedHostResolverProc> host_resolver_proc_;
  net::ScopedDefaultHostResolverProc scoped_host_resolver_proc_;
};

IN_PROC_BROWSER_TEST_F(BrowserTest2, NoTabsInPopups) {
  Browser::RegisterAppPrefs(L"Test");

  // We start with a normal browser with one tab.
  EXPECT_EQ(1, browser()->tab_count());

  // Open a popup browser with a single blank foreground tab.
  Browser* popup_browser = browser()->CreateForType(Browser::TYPE_POPUP,
                                                    browser()->profile());
  popup_browser->AddBlankTab(true);
  EXPECT_EQ(1, popup_browser->tab_count());

  // Now try opening another tab in the popup browser.
  AddTabWithURLParams params1(url, PageTransition::TYPED);
  popup_browser->AddTabWithURL(&params1);
  EXPECT_EQ(popup_browser, params1.target);

  // The popup should still only have one tab.
  EXPECT_EQ(1, popup_browser->tab_count());

  // The normal browser should now have two.
  EXPECT_EQ(2, browser()->tab_count());

  // Open an app frame browser with a single blank foreground tab.
  Browser* app_browser =
      browser()->CreateForApp(L"Test", browser()->profile(), false);
  app_browser->AddBlankTab(true);
  EXPECT_EQ(1, app_browser->tab_count());

  // Now try opening another tab in the app browser.
  AddTabWithURLParams params2(GURL(chrome::kAboutBlankURL),
                              PageTransition::TYPED);
  app_browser->AddTabWithURL(&params2);
  EXPECT_EQ(app_browser, params2.target);

  // The popup should still only have one tab.
  EXPECT_EQ(1, app_browser->tab_count());

  // The normal browser should now have three.
  EXPECT_EQ(3, browser()->tab_count());

  // Open an app frame popup browser with a single blank foreground tab.
  Browser* app_popup_browser =
      browser()->CreateForApp(L"Test", browser()->profile(), false);
  app_popup_browser->AddBlankTab(true);
  EXPECT_EQ(1, app_popup_browser->tab_count());

  // Now try opening another tab in the app popup browser.
  AddTabWithURLParams params3(GURL(chrome::kAboutBlankURL),
                              PageTransition::TYPED);
  app_popup_browser->AddTabWithURL(&params3);
  EXPECT_EQ(app_popup_browser, params3.target);

  // The popup should still only have one tab.
  EXPECT_EQ(1, app_popup_browser->tab_count());

  // The normal browser should now have four.
  EXPECT_EQ(4, browser()->tab_count());

  // Close the additional browsers.
  popup_browser->CloseAllTabs();
  app_browser->CloseAllTabs();
  app_popup_browser->CloseAllTabs();
}
#endif
