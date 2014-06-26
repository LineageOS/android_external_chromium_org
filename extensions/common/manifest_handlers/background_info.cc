// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/background_info.h"

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/switches.h"
#include "grit/extensions_strings.h"
#include "ui/base/l10n/l10n_util.h"

using base::ASCIIToUTF16;
using base::DictionaryValue;

namespace extensions {

namespace keys = manifest_keys;
namespace values = manifest_values;
namespace errors = manifest_errors;

namespace {

const char kBackground[] = "background";

static base::LazyInstance<BackgroundInfo> g_empty_background_info =
    LAZY_INSTANCE_INITIALIZER;

const BackgroundInfo& GetBackgroundInfo(const Extension* extension) {
  BackgroundInfo* info = static_cast<BackgroundInfo*>(
      extension->GetManifestData(kBackground));
  if (!info)
    return g_empty_background_info.Get();
  return *info;
}

}  // namespace

BackgroundInfo::BackgroundInfo()
    : is_persistent_(true),
      allow_js_access_(true) {
}

BackgroundInfo::~BackgroundInfo() {
}

// static
GURL BackgroundInfo::GetBackgroundURL(const Extension* extension) {
  const BackgroundInfo& info = GetBackgroundInfo(extension);
  if (info.background_scripts_.empty())
    return info.background_url_;
  return extension->GetResourceURL(kGeneratedBackgroundPageFilename);
}

// static
const std::vector<std::string>& BackgroundInfo::GetBackgroundScripts(
    const Extension* extension) {
  return GetBackgroundInfo(extension).background_scripts_;
}

// static
const std::string& BackgroundInfo::GetServiceWorkerScript(
    const Extension* extension) {
  return GetBackgroundInfo(extension).service_worker_script_;
}

// static
bool BackgroundInfo::HasBackgroundPage(const Extension* extension) {
  return GetBackgroundInfo(extension).has_background_page();
}

// static
bool BackgroundInfo::HasPersistentBackgroundPage(const Extension* extension)  {
  return GetBackgroundInfo(extension).has_persistent_background_page();
}

// static
bool BackgroundInfo::HasLazyBackgroundPage(const Extension* extension) {
  return GetBackgroundInfo(extension).has_lazy_background_page();
}

// static
bool BackgroundInfo::HasGeneratedBackgroundPage(const Extension* extension) {
  const BackgroundInfo& info = GetBackgroundInfo(extension);
  return !info.background_scripts_.empty();
}

// static
bool BackgroundInfo::HasServiceWorker(const Extension* extension) {
  return GetBackgroundInfo(extension).has_service_worker();
}

// static
bool BackgroundInfo::AllowJSAccess(const Extension* extension) {
  return GetBackgroundInfo(extension).allow_js_access_;
}

bool BackgroundInfo::Parse(const Extension* extension, base::string16* error) {
  const std::string& bg_scripts_key = extension->is_platform_app() ?
      keys::kPlatformAppBackgroundScripts : keys::kBackgroundScripts;
  const std::string& sw_scripts_key =
      extension->is_platform_app()
          ? keys::kPlatformAppServiceWorkerScript
          : "";  // TODO(scheib): Support extensions crbug.com/346885
  if (!LoadServiceWorkerScript(extension, sw_scripts_key, error) ||
      !LoadBackgroundScripts(extension, bg_scripts_key, error) ||
      !LoadBackgroundPage(extension, error) ||
      !LoadBackgroundPersistent(extension, error) ||
      !LoadAllowJSAccess(extension, error)) {
    return false;
  }

  int background_solution_sum = (background_url_.is_valid() ? 1 : 0) +
                                (!background_scripts_.empty() ? 1 : 0) +
                                (has_service_worker() ? 1 : 0);
  if (background_solution_sum > 1) {
    *error = ASCIIToUTF16(errors::kInvalidBackgroundCombination);
    return false;
  }

  return true;
}

bool BackgroundInfo::LoadServiceWorkerScript(const Extension* extension,
                                             const std::string& key,
                                             base::string16* error) {
  const base::Value* service_worker_script_value = NULL;
  if (!extension->manifest()->Get(key, &service_worker_script_value))
    return true;

  if (!CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kEnableExperimentalWebPlatformFeatures)) {
    *error = ASCIIToUTF16(errors::kServiceWorkerRequiresFlag);
    return false;
  }

  CHECK(service_worker_script_value);
  if (!service_worker_script_value->GetAsString(&service_worker_script_)) {
    *error = ASCIIToUTF16(errors::kInvalidServiceWorkerScript);
    return false;
  }
  return true;
}

bool BackgroundInfo::LoadBackgroundScripts(const Extension* extension,
                                           const std::string& key,
                                           base::string16* error) {
  const base::Value* background_scripts_value = NULL;
  if (!extension->manifest()->Get(key, &background_scripts_value))
    return true;

  CHECK(background_scripts_value);
  if (background_scripts_value->GetType() != base::Value::TYPE_LIST) {
    *error = ASCIIToUTF16(errors::kInvalidBackgroundScripts);
    return false;
  }

  const base::ListValue* background_scripts = NULL;
  background_scripts_value->GetAsList(&background_scripts);
  for (size_t i = 0; i < background_scripts->GetSize(); ++i) {
    std::string script;
    if (!background_scripts->GetString(i, &script)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidBackgroundScript, base::IntToString(i));
      return false;
    }
    background_scripts_.push_back(script);
  }

  return true;
}

bool BackgroundInfo::LoadBackgroundPage(const Extension* extension,
                                        const std::string& key,
                                        base::string16* error) {
  const base::Value* background_page_value = NULL;
  if (!extension->manifest()->Get(key, &background_page_value))
    return true;

  std::string background_str;
  if (!background_page_value->GetAsString(&background_str)) {
    *error = ASCIIToUTF16(errors::kInvalidBackground);
    return false;
  }

  if (extension->is_hosted_app()) {
    background_url_ = GURL(background_str);

    if (!PermissionsParser::HasAPIPermission(extension,
                                             APIPermission::kBackground)) {
      *error = ASCIIToUTF16(errors::kBackgroundPermissionNeeded);
      return false;
    }
    // Hosted apps require an absolute URL.
    if (!background_url_.is_valid()) {
      *error = ASCIIToUTF16(errors::kInvalidBackgroundInHostedApp);
      return false;
    }

    if (!(background_url_.SchemeIs("https") ||
          (CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kAllowHTTPBackgroundPage) &&
           background_url_.SchemeIs("http")))) {
      *error = ASCIIToUTF16(errors::kInvalidBackgroundInHostedApp);
      return false;
    }
  } else {
    background_url_ = extension->GetResourceURL(background_str);
  }

  return true;
}

bool BackgroundInfo::LoadBackgroundPage(const Extension* extension,
                                        base::string16* error) {
  if (extension->is_platform_app()) {
    return LoadBackgroundPage(
        extension, keys::kPlatformAppBackgroundPage, error);
  }

  if (!LoadBackgroundPage(extension, keys::kBackgroundPage, error))
    return false;
  if (background_url_.is_empty())
    return LoadBackgroundPage(extension, keys::kBackgroundPageLegacy, error);
  return true;
}

bool BackgroundInfo::LoadBackgroundPersistent(const Extension* extension,
                                              base::string16* error) {
  if (extension->is_platform_app()) {
    is_persistent_ = false;
    return true;
  }

  const base::Value* background_persistent = NULL;
  if (!extension->manifest()->Get(keys::kBackgroundPersistent,
                                  &background_persistent))
    return true;

  if (!background_persistent->GetAsBoolean(&is_persistent_)) {
    *error = ASCIIToUTF16(errors::kInvalidBackgroundPersistent);
    return false;
  }

  if (!has_background_page()) {
    *error = ASCIIToUTF16(errors::kInvalidBackgroundPersistentNoPage);
    return false;
  }

  return true;
}

bool BackgroundInfo::LoadAllowJSAccess(const Extension* extension,
                                       base::string16* error) {
  const base::Value* allow_js_access = NULL;
  if (!extension->manifest()->Get(keys::kBackgroundAllowJsAccess,
                                  &allow_js_access))
    return true;

  if (!allow_js_access->IsType(base::Value::TYPE_BOOLEAN) ||
      !allow_js_access->GetAsBoolean(&allow_js_access_)) {
    *error = ASCIIToUTF16(errors::kInvalidBackgroundAllowJsAccess);
    return false;
  }

  return true;
}

BackgroundManifestHandler::BackgroundManifestHandler() {
}

BackgroundManifestHandler::~BackgroundManifestHandler() {
}

bool BackgroundManifestHandler::Parse(Extension* extension,
                                      base::string16* error) {
  scoped_ptr<BackgroundInfo> info(new BackgroundInfo);
  if (!info->Parse(extension, error))
    return false;

  // Platform apps must have background pages or service workers.
  if (extension->is_platform_app() && !info->has_background_page() &&
      !info->has_service_worker()) {
    *error = ASCIIToUTF16(errors::kBackgroundRequiredForPlatformApps);
    return false;
  }
  // Lazy background pages are incompatible with the webRequest API.
  if (info->has_lazy_background_page() &&
      PermissionsParser::HasAPIPermission(extension,
                                          APIPermission::kWebRequest)) {
    *error = ASCIIToUTF16(errors::kWebRequestConflictsWithLazyBackground);
    return false;
  }

  extension->SetManifestData(kBackground, info.release());
  return true;
}

bool BackgroundManifestHandler::Validate(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) const {
  // Validate that background scripts exist.
  const std::vector<std::string>& background_scripts =
      BackgroundInfo::GetBackgroundScripts(extension);
  for (size_t i = 0; i < background_scripts.size(); ++i) {
    if (!base::PathExists(
            extension->GetResource(background_scripts[i]).GetFilePath())) {
      *error = l10n_util::GetStringFUTF8(
          IDS_EXTENSION_LOAD_BACKGROUND_SCRIPT_FAILED,
          base::UTF8ToUTF16(background_scripts[i]));
      return false;
    }
  }

  // Validate background page location, except for hosted apps, which should use
  // an external URL. Background page for hosted apps are verified when the
  // extension is created (in Extension::InitFromValue)
  if (BackgroundInfo::HasBackgroundPage(extension) &&
      !extension->is_hosted_app() && background_scripts.empty()) {
    base::FilePath page_path = file_util::ExtensionURLToRelativeFilePath(
        BackgroundInfo::GetBackgroundURL(extension));
    const base::FilePath path = extension->GetResource(page_path).GetFilePath();
    if (path.empty() || !base::PathExists(path)) {
      *error =
          l10n_util::GetStringFUTF8(
              IDS_EXTENSION_LOAD_BACKGROUND_PAGE_FAILED,
              page_path.LossyDisplayName());
      return false;
    }
  }
  return true;
}

bool BackgroundManifestHandler::AlwaysParseForType(Manifest::Type type) const {
  return type == Manifest::TYPE_PLATFORM_APP;
}

const std::vector<std::string> BackgroundManifestHandler::Keys() const {
  static const char* keys[] = {keys::kBackgroundAllowJsAccess,
                               keys::kBackgroundPage,
                               keys::kBackgroundPageLegacy,
                               keys::kBackgroundPersistent,
                               keys::kBackgroundScripts,
                               keys::kPlatformAppBackgroundPage,
                               keys::kPlatformAppBackgroundScripts,
                               keys::kPlatformAppServiceWorkerScript};
  return std::vector<std::string>(keys, keys + arraysize(keys));
}

}  // namespace extensions
