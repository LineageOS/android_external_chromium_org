// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "ui/file_manager/grit/file_manager_resources.h"
#endif

namespace extensions {

typedef testing::Test ChromeComponentExtensionResourceManagerTest;

// Tests IsComponentExtensionResource function.
TEST_F(ChromeComponentExtensionResourceManagerTest,
       IsComponentExtensionResource) {
  ComponentExtensionResourceManager* resource_manager =
      ExtensionsBrowserClient::Get()->GetComponentExtensionResourceManager();
  ASSERT_TRUE(resource_manager);

  // Get the extension test data path.
  base::FilePath test_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &test_path));
  test_path = test_path.AppendASCII("extensions").AppendASCII("file_manager");

  // Load the manifest data.
  std::string error;
  scoped_ptr<base::DictionaryValue> manifest(file_util::LoadManifest(
      test_path, FILE_PATH_LITERAL("app.json"), &error));
  ASSERT_TRUE(manifest.get()) << error;

  // Build a path inside Chrome's resources directory where a component
  // extension might be installed.
  base::FilePath resources_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_RESOURCES, &resources_path));
  resources_path = resources_path.AppendASCII("file_manager");

  // Create a simulated component extension.
  scoped_refptr<Extension> extension = Extension::Create(resources_path,
                                                         Manifest::COMPONENT,
                                                         *manifest,
                                                         Extension::NO_FLAGS,
                                                         &error);
  ASSERT_TRUE(extension.get());

  // Load one of the icons.
  ExtensionResource resource =
      IconsInfo::GetIconResource(extension.get(),
                                 extension_misc::EXTENSION_ICON_BITTY,
                                 ExtensionIconSet::MATCH_EXACTLY);

#if defined(OS_CHROMEOS)
  // The resource is a component resource.
  int resource_id = 0;
  ASSERT_TRUE(resource_manager->IsComponentExtensionResource(
      extension->path(), resource.relative_path(), &resource_id));
  ASSERT_EQ(IDR_FILE_MANAGER_ICON_16, resource_id);
#endif
}

}  // namespace extensions
