// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/advanced_options_handler.h"

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/google/google_util.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service.h"
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service_factory.h"
#include "chrome/browser/printing/cloud_print/cloud_print_setup_flow.h"
#include "chrome/browser/printing/cloud_print/cloud_print_url.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/service/service_process_control.h"
#include "chrome/browser/ui/options/options_util.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "content/browser/download/download_manager.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "content/browser/user_metrics.h"
#include "content/common/content_notification_types.h"
#include "content/common/notification_details.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(OS_CHROMEOS)
#include "chrome/browser/printing/cloud_print/cloud_print_setup_handler.h"
#include "chrome/browser/ui/webui/options/advanced_options_utils.h"
#endif

AdvancedOptionsHandler::AdvancedOptionsHandler() {

#if(!defined(GOOGLE_CHROME_BUILD) && defined(OS_WIN))
  // On Windows, we need the PDF plugin which is only guaranteed to exist on
  // Google Chrome builds. Use a command-line switch for Windows non-Google
  //  Chrome builds.
  cloud_print_proxy_ui_enabled_ = CommandLine::ForCurrentProcess()->HasSwitch(
         switches::kEnableCloudPrintProxy);
#elif(!defined(OS_CHROMEOS))
  // Always enabled for Mac, Linux and Google Chrome Windows builds.
  // Never enabled for Chrome OS, we don't even need to indicate it.
  cloud_print_proxy_ui_enabled_ = true;
#endif
}

AdvancedOptionsHandler::~AdvancedOptionsHandler() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_folder_dialog_.get())
    select_folder_dialog_->ListenerDestroyed();
}

void AdvancedOptionsHandler::GetLocalizedValues(
    DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  static OptionsStringResource resources[] = {
    { "downloadLocationGroupName",
      IDS_OPTIONS_DOWNLOADLOCATION_GROUP_NAME },
    { "downloadLocationChangeButton",
      IDS_OPTIONS_DOWNLOADLOCATION_CHANGE_BUTTON },
    { "downloadLocationBrowseTitle",
      IDS_OPTIONS_DOWNLOADLOCATION_BROWSE_TITLE },
    { "downloadLocationBrowseWindowTitle",
      IDS_OPTIONS_DOWNLOADLOCATION_BROWSE_WINDOW_TITLE },
    { "downloadLocationAskForSaveLocation",
      IDS_OPTIONS_DOWNLOADLOCATION_ASKFORSAVELOCATION },
    { "autoOpenFileTypesInfo",
      IDS_OPTIONS_OPEN_FILE_TYPES_AUTOMATICALLY },
    { "autoOpenFileTypesResetToDefault",
      IDS_OPTIONS_AUTOOPENFILETYPES_RESETTODEFAULT },
    { "translateEnableTranslate",
      IDS_OPTIONS_TRANSLATE_ENABLE_TRANSLATE },
    { "certificatesManageButton",
      IDS_OPTIONS_CERTIFICATES_MANAGE_BUTTON },
    { "proxiesLabel",
      IDS_OPTIONS_PROXIES_LABEL },
#if !defined(OS_CHROMEOS)
    { "proxiesConfigureButton",
      IDS_OPTIONS_PROXIES_CONFIGURE_BUTTON },
#endif
    { "safeBrowsingEnableProtection",
      IDS_OPTIONS_SAFEBROWSING_ENABLEPROTECTION },
    { "sslGroupDescription",
      IDS_OPTIONS_SSL_GROUP_DESCRIPTION },
    { "sslCheckRevocation",
      IDS_OPTIONS_SSL_CHECKREVOCATION },
    { "sslUseSSL3",
      IDS_OPTIONS_SSL_USESSL3 },
    { "sslUseTLS1",
      IDS_OPTIONS_SSL_USETLS1 },
    { "networkPredictionEnabledDescription",
      IDS_NETWORK_PREDICTION_ENABLED_DESCRIPTION },
    { "privacyContentSettingsButton",
      IDS_OPTIONS_PRIVACY_CONTENT_SETTINGS_BUTTON },
    { "privacyClearDataButton",
      IDS_OPTIONS_PRIVACY_CLEAR_DATA_BUTTON },
    { "linkDoctorPref",
      IDS_OPTIONS_LINKDOCTOR_PREF },
    { "suggestPref",
      IDS_OPTIONS_SUGGEST_PREF },
    { "tabsToLinksPref",
      IDS_OPTIONS_TABS_TO_LINKS_PREF },
    { "fontSettingsInfo",
      IDS_OPTIONS_FONTSETTINGS_INFO },
    { "defaultZoomLevelLabel",
      IDS_OPTIONS_DEFAULT_ZOOM_LEVEL_LABEL },
    { "defaultFontSizeLabel",
      IDS_OPTIONS_DEFAULT_FONT_SIZE_LABEL },
    { "fontSizeLabelVerySmall",
      IDS_OPTIONS_FONT_SIZE_LABEL_VERY_SMALL },
    { "fontSizeLabelSmall",
      IDS_OPTIONS_FONT_SIZE_LABEL_SMALL },
    { "fontSizeLabelMedium",
      IDS_OPTIONS_FONT_SIZE_LABEL_MEDIUM },
    { "fontSizeLabelLarge",
      IDS_OPTIONS_FONT_SIZE_LABEL_LARGE },
    { "fontSizeLabelVeryLarge",
      IDS_OPTIONS_FONT_SIZE_LABEL_VERY_LARGE },
    { "fontSizeLabelCustom",
      IDS_OPTIONS_FONT_SIZE_LABEL_CUSTOM },
    { "fontSettingsCustomizeFontsButton",
      IDS_OPTIONS_FONTSETTINGS_CUSTOMIZE_FONTS_BUTTON },
    { "languageAndSpellCheckSettingsButton",
      IDS_OPTIONS_LANGUAGE_AND_SPELLCHECK_BUTTON },
    { "advancedSectionTitlePrivacy",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_PRIVACY },
    { "advancedSectionTitleContent",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_CONTENT },
    { "advancedSectionTitleSecurity",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_SECURITY },
    { "advancedSectionTitleNetwork",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_NETWORK },
    { "advancedSectionTitleTranslate",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_TRANSLATE },
    { "translateEnableTranslate",
      IDS_OPTIONS_TRANSLATE_ENABLE_TRANSLATE },
    { "enableLogging",
      IDS_OPTIONS_ENABLE_LOGGING },
    { "improveBrowsingExperience",
      IDS_OPTIONS_IMPROVE_BROWSING_EXPERIENCE },
    { "disableWebServices",
      IDS_OPTIONS_DISABLE_WEB_SERVICES },
#if defined(OS_CHROMEOS)
    { "cloudPrintChromeosOptionLabel",
      IDS_CLOUD_PRINT_CHROMEOS_OPTION_LABEL },
    { "cloudPrintChromeosOptionButton",
      IDS_CLOUD_PRINT_CHROMEOS_OPTION_BUTTON },
#endif
    { "cloudPrintOptionsStaticLabel",
      IDS_CLOUD_PRINT_SETUP_DIALOG_TITLE },
    { "cloudPrintProxyEnabledManageButton",
      IDS_OPTIONS_CLOUD_PRINT_PROXY_ENABLED_MANAGE_BUTTON },
    { "advancedSectionTitleCloudPrint",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_CLOUD_PRINT },
#if !defined(OS_CHROMEOS)
    { "cloudPrintProxyDisabledLabel",
      IDS_OPTIONS_CLOUD_PRINT_PROXY_DISABLED_LABEL },
    { "cloudPrintProxyDisabledButton",
      IDS_OPTIONS_CLOUD_PRINT_PROXY_DISABLED_BUTTON },
    { "cloudPrintProxyEnabledButton",
      IDS_OPTIONS_CLOUD_PRINT_PROXY_ENABLED_BUTTON },
    { "cloudPrintProxyEnablingButton",
      IDS_OPTIONS_CLOUD_PRINT_PROXY_ENABLING_BUTTON },
#endif
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
    { "advancedSectionTitleBackground",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_BACKGROUND },
    { "backgroundModeCheckbox",
      IDS_OPTIONS_BACKGROUND_ENABLE_BACKGROUND_MODE },
#endif
  };

  RegisterStrings(localized_strings, resources, arraysize(resources));
  RegisterTitle(localized_strings, "advancedPage",
                IDS_OPTIONS_ADVANCED_TAB_LABEL);

  localized_strings->SetString("privacyLearnMoreURL",
      google_util::AppendGoogleLocaleParam(
          GURL(chrome::kPrivacyLearnMoreURL)).spec());

#if defined(OS_CHROMEOS)
  localized_strings->SetString("cloudPrintLearnMoreURL",
      google_util::AppendGoogleLocaleParam(
          GURL(chrome::kCloudPrintLearnMoreURL)).spec());
#endif
}

void AdvancedOptionsHandler::Initialize() {
  DCHECK(web_ui_);
  SetupMetricsReportingCheckbox();
  SetupMetricsReportingSettingVisibility();
  SetupFontSizeLabel();
  SetupDownloadLocationPath();
  SetupPromptForDownload();
  SetupAutoOpenFileTypesDisabledAttribute();
  SetupProxySettingsSection();
  SetupSSLConfigSettings();
#if !defined(OS_CHROMEOS)
  if (cloud_print_proxy_ui_enabled_) {
    SetupCloudPrintProxySection();
    RefreshCloudPrintStatusFromService();
  } else {
    RemoveCloudPrintProxySection();
  }
#endif
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
  SetupBackgroundModeSettings();
#endif

}

WebUIMessageHandler* AdvancedOptionsHandler::Attach(WebUI* web_ui) {
  // Call through to superclass.
  WebUIMessageHandler* handler = OptionsPageUIHandler::Attach(web_ui);

  // Register for preferences that we need to observe manually.  These have
  // special behaviors that aren't handled by the standard prefs UI.
  DCHECK(web_ui_);
  PrefService* prefs = Profile::FromWebUI(web_ui_)->GetPrefs();
#if !defined(OS_CHROMEOS)
  enable_metrics_recording_.Init(prefs::kMetricsReportingEnabled,
                                 g_browser_process->local_state(), this);
  cloud_print_proxy_email_.Init(prefs::kCloudPrintEmail, prefs, this);
  cloud_print_proxy_enabled_.Init(prefs::kCloudPrintProxyEnabled, prefs, this);
#endif

  rev_checking_enabled_.Init(prefs::kCertRevocationCheckingEnabled,
                             g_browser_process->local_state(), this);
  ssl3_enabled_.Init(prefs::kSSL3Enabled, g_browser_process->local_state(),
                     this);
  tls1_enabled_.Init(prefs::kTLS1Enabled, g_browser_process->local_state(),
                     this);

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
  background_mode_enabled_.Init(prefs::kBackgroundModeEnabled,
                                g_browser_process->local_state(),
                                this);
#endif

  default_download_location_.Init(prefs::kDownloadDefaultDirectory,
                                  prefs, this);
  ask_for_save_location_.Init(prefs::kPromptForDownload, prefs, this);
  allow_file_selection_dialogs_.Init(prefs::kAllowFileSelectionDialogs,
                                     g_browser_process->local_state(), this);
  auto_open_files_.Init(prefs::kDownloadExtensionsToOpen, prefs, this);
  default_font_size_.Init(prefs::kWebKitDefaultFontSize, prefs, this);
  proxy_prefs_.reset(
      PrefSetObserver::CreateProxyPrefSetObserver(prefs, this));

  // Return result from the superclass.
  return handler;
}

void AdvancedOptionsHandler::RegisterMessages() {
  // Setup handlers specific to this panel.
  DCHECK(web_ui_);
  web_ui_->RegisterMessageCallback("selectDownloadLocation",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleSelectDownloadLocation));
  web_ui_->RegisterMessageCallback("promptForDownloadAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandlePromptForDownload));
  web_ui_->RegisterMessageCallback("autoOpenFileTypesAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleAutoOpenButton));
  web_ui_->RegisterMessageCallback("defaultFontSizeAction",
      NewCallback(this, &AdvancedOptionsHandler::HandleDefaultFontSize));
#if !defined(OS_CHROMEOS)
  web_ui_->RegisterMessageCallback("metricsReportingCheckboxAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleMetricsReportingCheckbox));
#endif
#if !defined(USE_NSS) && !defined(USE_OPENSSL)
  web_ui_->RegisterMessageCallback("showManageSSLCertificates",
      NewCallback(this,
                  &AdvancedOptionsHandler::ShowManageSSLCertificates));
#endif
  web_ui_->RegisterMessageCallback("showCloudPrintManagePage",
      NewCallback(this,
                  &AdvancedOptionsHandler::ShowCloudPrintManagePage));
#if !defined(OS_CHROMEOS)
  if (cloud_print_proxy_ui_enabled_) {
    web_ui_->RegisterMessageCallback("showCloudPrintSetupDialog",
        NewCallback(this,
                    &AdvancedOptionsHandler::ShowCloudPrintSetupDialog));
    web_ui_->RegisterMessageCallback("disableCloudPrintProxy",
        NewCallback(this,
                    &AdvancedOptionsHandler::HandleDisableCloudPrintProxy));
  }
  web_ui_->RegisterMessageCallback("showNetworkProxySettings",
      NewCallback(this,
                  &AdvancedOptionsHandler::ShowNetworkProxySettings));
#endif
  web_ui_->RegisterMessageCallback("checkRevocationCheckboxAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleCheckRevocationCheckbox));
  web_ui_->RegisterMessageCallback("useSSL3CheckboxAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleUseSSL3Checkbox));
  web_ui_->RegisterMessageCallback("useTLS1CheckboxAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleUseTLS1Checkbox));
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
  web_ui_->RegisterMessageCallback("backgroundModeAction",
      NewCallback(this,
                  &AdvancedOptionsHandler::HandleBackgroundModeCheckbox));
#endif
}

void AdvancedOptionsHandler::Observe(int type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_PREF_CHANGED) {
    std::string* pref_name = Details<std::string>(details).ptr();
    if ((*pref_name == prefs::kDownloadDefaultDirectory) ||
        (*pref_name == prefs::kPromptForDownload) ||
        (*pref_name == prefs::kAllowFileSelectionDialogs)) {
      SetupDownloadLocationPath();
      SetupPromptForDownload();
    } else if (*pref_name == prefs::kDownloadExtensionsToOpen) {
      SetupAutoOpenFileTypesDisabledAttribute();
    } else if (proxy_prefs_->IsObserved(*pref_name)) {
      SetupProxySettingsSection();
    } else if ((*pref_name == prefs::kCloudPrintEmail) ||
               (*pref_name == prefs::kCloudPrintProxyEnabled)) {
#if !defined(OS_CHROMEOS)
      if (cloud_print_proxy_ui_enabled_)
        SetupCloudPrintProxySection();
#endif
    } else if (*pref_name == prefs::kWebKitDefaultFontSize) {
      SetupFontSizeLabel();
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
    } else if (*pref_name == prefs::kBackgroundModeEnabled) {
      SetupBackgroundModeSettings();
#endif
    }
  }
}

void AdvancedOptionsHandler::HandleSelectDownloadLocation(
    const ListValue* args) {
  PrefService* pref_service = Profile::FromWebUI(web_ui_)->GetPrefs();
  select_folder_dialog_ = SelectFileDialog::Create(this);
  select_folder_dialog_->SelectFile(
      SelectFileDialog::SELECT_FOLDER,
      l10n_util::GetStringUTF16(IDS_OPTIONS_DOWNLOADLOCATION_BROWSE_TITLE),
      pref_service->GetFilePath(prefs::kDownloadDefaultDirectory),
      NULL, 0, FILE_PATH_LITERAL(""), web_ui_->tab_contents(),
      web_ui_->tab_contents()->view()->GetTopLevelNativeWindow(), NULL);
}

void AdvancedOptionsHandler::HandlePromptForDownload(
    const ListValue* args) {
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  ask_for_save_location_.SetValue(checked_str == "true");
}

void AdvancedOptionsHandler::FileSelected(const FilePath& path, int index,
                                          void* params) {
  UserMetrics::RecordAction(UserMetricsAction("Options_SetDownloadDirectory"));
  default_download_location_.SetValue(path);
  SetupDownloadLocationPath();
}

void AdvancedOptionsHandler::OnCloudPrintSetupClosed() {
#if !defined(OS_CHROMEOS)
  if (cloud_print_proxy_ui_enabled_)
    SetupCloudPrintProxySection();
#endif
}

void AdvancedOptionsHandler::HandleAutoOpenButton(const ListValue* args) {
  UserMetrics::RecordAction(UserMetricsAction("Options_ResetAutoOpenFiles"));
  DownloadManager* manager =
      web_ui_->tab_contents()->browser_context()->GetDownloadManager();
  if (manager)
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();
}

void AdvancedOptionsHandler::HandleMetricsReportingCheckbox(
    const ListValue* args) {
#if defined(GOOGLE_CHROME_BUILD) && !defined(OS_CHROMEOS)
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  bool enabled = checked_str == "true";
  UserMetrics::RecordAction(
      enabled ?
          UserMetricsAction("Options_MetricsReportingCheckbox_Enable") :
          UserMetricsAction("Options_MetricsReportingCheckbox_Disable"));
  bool is_enabled = OptionsUtil::ResolveMetricsReportingEnabled(enabled);
  enable_metrics_recording_.SetValue(is_enabled);
  SetupMetricsReportingCheckbox();
#endif
}

void AdvancedOptionsHandler::HandleDefaultFontSize(const ListValue* args) {
  int font_size;
  if (ExtractIntegerValue(args, &font_size)) {
    if (font_size > 0) {
      default_font_size_.SetValue(font_size);
      SetupFontSizeLabel();
    }
  }
}

void AdvancedOptionsHandler::HandleCheckRevocationCheckbox(
    const ListValue* args) {
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  bool enabled = checked_str == "true";
  UserMetrics::RecordAction(
      enabled ?
          UserMetricsAction("Options_CheckCertRevocation_Enable") :
          UserMetricsAction("Options_CheckCertRevocation_Disable"));
  rev_checking_enabled_.SetValue(enabled);
}

void AdvancedOptionsHandler::HandleUseSSL3Checkbox(const ListValue* args) {
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  bool enabled = checked_str == "true";
  UserMetrics::RecordAction(
      enabled ?
          UserMetricsAction("Options_SSL3_Enable") :
          UserMetricsAction("Options_SSL3_Disable"));
  ssl3_enabled_.SetValue(enabled);
}

void AdvancedOptionsHandler::HandleUseTLS1Checkbox(const ListValue* args) {
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  bool enabled = checked_str == "true";
  UserMetrics::RecordAction(
      enabled ?
          UserMetricsAction("Options_TLS1_Enable") :
          UserMetricsAction("Options_TLS1_Disable"));
  tls1_enabled_.SetValue(enabled);
}

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
void AdvancedOptionsHandler::HandleBackgroundModeCheckbox(
    const ListValue* args) {
  std::string checked_str = UTF16ToUTF8(ExtractStringValue(args));
  bool enabled = checked_str == "true";
  UserMetrics::RecordAction(enabled ?
      UserMetricsAction("Options_BackgroundMode_Enable") :
      UserMetricsAction("Options_BackgroundMode_Disable"));
  background_mode_enabled_.SetValue(enabled);
}

void AdvancedOptionsHandler::SetupBackgroundModeSettings() {
    base::FundamentalValue checked(background_mode_enabled_.GetValue());
    web_ui_->CallJavascriptFunction(
        "options.AdvancedOptions.SetBackgroundModeCheckboxState", checked);
}
#endif

#if !defined(OS_CHROMEOS)
void AdvancedOptionsHandler::ShowNetworkProxySettings(const ListValue* args) {
  UserMetrics::RecordAction(UserMetricsAction("Options_ShowProxySettings"));
  AdvancedOptionsUtilities::ShowNetworkProxySettings(web_ui_->tab_contents());
}
#endif

#if !defined(USE_NSS) && !defined(USE_OPENSSL)
void AdvancedOptionsHandler::ShowManageSSLCertificates(const ListValue* args) {
  UserMetrics::RecordAction(UserMetricsAction("Options_ManageSSLCertificates"));
  AdvancedOptionsUtilities::ShowManageSSLCertificates(web_ui_->tab_contents());
}
#endif

void AdvancedOptionsHandler::ShowCloudPrintManagePage(const ListValue* args) {
  UserMetrics::RecordAction(UserMetricsAction("Options_ManageCloudPrinters"));
  // Open a new tab in the current window for the management page.
  Profile* profile = Profile::FromWebUI(web_ui_);
  web_ui_->tab_contents()->OpenURL(
      CloudPrintURL(profile).GetCloudPrintServiceManageURL(),
      GURL(), NEW_FOREGROUND_TAB, PageTransition::LINK);
}

#if !defined(OS_CHROMEOS)
void AdvancedOptionsHandler::ShowCloudPrintSetupDialog(const ListValue* args) {
  UserMetrics::RecordAction(UserMetricsAction("Options_EnableCloudPrintProxy"));
  // Open the connector enable page in the current tab.
  Profile* profile = Profile::FromWebUI(web_ui_);
  web_ui_->tab_contents()->OpenURL(
      CloudPrintURL(profile).GetCloudPrintServiceEnableURL(
          CloudPrintProxyServiceFactory::GetForProfile(profile)->proxy_id()),
      GURL(), CURRENT_TAB, PageTransition::LINK);
}

void AdvancedOptionsHandler::HandleDisableCloudPrintProxy(
    const ListValue* args) {
  UserMetrics::RecordAction(
      UserMetricsAction("Options_DisableCloudPrintProxy"));
  CloudPrintProxyServiceFactory::GetForProfile(Profile::FromWebUI(web_ui_))->
      DisableForUser();
}

void AdvancedOptionsHandler::RefreshCloudPrintStatusFromService() {
  DCHECK(web_ui_);
  if (cloud_print_proxy_ui_enabled_)
    CloudPrintProxyServiceFactory::GetForProfile(Profile::FromWebUI(web_ui_))->
        RefreshStatusFromService();
}

void AdvancedOptionsHandler::SetupCloudPrintProxySection() {
  Profile* profile = Profile::FromWebUI(web_ui_);
  if (!CloudPrintProxyServiceFactory::GetForProfile(profile)) {
    cloud_print_proxy_ui_enabled_ = false;
    RemoveCloudPrintProxySection();
    return;
  }

  bool cloud_print_proxy_allowed =
      !cloud_print_proxy_enabled_.IsManaged() ||
      cloud_print_proxy_enabled_.GetValue();
  base::FundamentalValue allowed(cloud_print_proxy_allowed);

  std::string email;
  if (profile->GetPrefs()->HasPrefPath(prefs::kCloudPrintEmail) &&
      cloud_print_proxy_allowed) {
    email = profile->GetPrefs()->GetString(prefs::kCloudPrintEmail);
  }
  base::FundamentalValue disabled(email.empty());

  string16 label_str;
  if (email.empty()) {
    label_str = l10n_util::GetStringUTF16(
        IDS_OPTIONS_CLOUD_PRINT_PROXY_DISABLED_LABEL);
  } else {
    label_str = l10n_util::GetStringFUTF16(
        IDS_OPTIONS_CLOUD_PRINT_PROXY_ENABLED_LABEL, UTF8ToUTF16(email));
  }
  StringValue label(label_str);

  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetupCloudPrintProxySection",
      disabled, label, allowed);
}

void AdvancedOptionsHandler::RemoveCloudPrintProxySection() {
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.RemoveCloudPrintProxySection");
}

#endif

void AdvancedOptionsHandler::SetupMetricsReportingCheckbox() {
#if defined(GOOGLE_CHROME_BUILD) && !defined(OS_CHROMEOS)
  base::FundamentalValue checked(enable_metrics_recording_.GetValue());
  base::FundamentalValue disabled(enable_metrics_recording_.IsManaged());
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetMetricsReportingCheckboxState", checked,
      disabled);
#endif
}

void AdvancedOptionsHandler::SetupMetricsReportingSettingVisibility() {
#if defined(GOOGLE_CHROME_BUILD) && defined(OS_CHROMEOS)
  // Don't show the reporting setting if we are in the guest mode.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kGuestSession)) {
    base::FundamentalValue visible(false);
    web_ui_->CallJavascriptFunction(
        "options.AdvancedOptions.SetMetricsReportingSettingVisibility",
        visible);
  }
#endif
}

void AdvancedOptionsHandler::SetupFontSizeLabel() {
  // We're only interested in integer values, so convert to int.
  base::FundamentalValue font_size(default_font_size_.GetValue());
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetFontSize", font_size);
}

void AdvancedOptionsHandler::SetupDownloadLocationPath() {
  StringValue value(default_download_location_.GetValue().value());
  // In case allow_file_selection_dialogs_ is false, we will not display any
  // file-selection dialogs but show an InfoBar. That is why we can disable
  // the DownloadLocationPath-Chooser right-away.
  base::FundamentalValue disabled(default_download_location_.IsManaged() ||
                            !allow_file_selection_dialogs_.GetValue());
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetDownloadLocationPath", value, disabled);
}

void AdvancedOptionsHandler::SetupPromptForDownload() {
  base::FundamentalValue checked(ask_for_save_location_.GetValue());
  // If either the DownloadDirectory is managed or if file-selection dialogs are
  // disallowed then |ask_for_save_location_| must currently be false and cannot
  // be changed.
  base::FundamentalValue disabled(default_download_location_.IsManaged() ||
                            !allow_file_selection_dialogs_.GetValue());
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetPromptForDownload", checked, disabled);
}

void AdvancedOptionsHandler::SetupAutoOpenFileTypesDisabledAttribute() {
  // Set the enabled state for the AutoOpenFileTypesResetToDefault button.
  // We enable the button if the user has any auto-open file types registered.
  DownloadManager* manager =
      web_ui_->tab_contents()->browser_context()->GetDownloadManager();
  bool disabled = !(manager &&
      DownloadPrefs::FromDownloadManager(manager)->IsAutoOpenUsed());
  base::FundamentalValue value(disabled);
  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetAutoOpenFileTypesDisabledAttribute", value);
}

void AdvancedOptionsHandler::SetupProxySettingsSection() {
  // Disable the button if proxy settings are managed by a sysadmin or
  // overridden by an extension.
  PrefService* pref_service = Profile::FromWebUI(web_ui_)->GetPrefs();
  const PrefService::Preference* proxy_config =
      pref_service->FindPreference(prefs::kProxy);
  bool is_extension_controlled = (proxy_config &&
                                  proxy_config->IsExtensionControlled());

  base::FundamentalValue disabled(proxy_prefs_->IsManaged() ||
                            is_extension_controlled);

  // Get the appropriate info string to describe the button.
  string16 label_str;
  if (is_extension_controlled) {
    label_str = l10n_util::GetStringUTF16(IDS_OPTIONS_EXTENSION_PROXIES_LABEL);
  } else {
    label_str = l10n_util::GetStringFUTF16(IDS_OPTIONS_SYSTEM_PROXIES_LABEL,
        l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  }
  StringValue label(label_str);

  web_ui_->CallJavascriptFunction(
      "options.AdvancedOptions.SetupProxySettingsSection", disabled, label);
}

void AdvancedOptionsHandler::SetupSSLConfigSettings() {
  {
    base::FundamentalValue checked(rev_checking_enabled_.GetValue());
    base::FundamentalValue disabled(rev_checking_enabled_.IsManaged());
    web_ui_->CallJavascriptFunction(
        "options.AdvancedOptions.SetCheckRevocationCheckboxState", checked,
        disabled);
  }
  {
    base::FundamentalValue checked(ssl3_enabled_.GetValue());
    base::FundamentalValue disabled(ssl3_enabled_.IsManaged());
    web_ui_->CallJavascriptFunction(
        "options.AdvancedOptions.SetUseSSL3CheckboxState", checked, disabled);
  }
  {
    base::FundamentalValue checked(tls1_enabled_.GetValue());
    base::FundamentalValue disabled(tls1_enabled_.IsManaged());
    web_ui_->CallJavascriptFunction(
        "options.AdvancedOptions.SetUseTLS1CheckboxState", checked, disabled);
  }
}
