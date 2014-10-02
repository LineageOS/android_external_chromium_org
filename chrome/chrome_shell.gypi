# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This GYPI allows independent customization of the chrome shell in a manner
# similar to content shell (in content_shell.gypi). Notably, this file does
# NOT contain chrome_android_core, which is independent of the chrome shell
# and should be separately customized.
{
  'variables': {
    'apk_name': 'ChromeShell',
    'manifest_package_name': 'org.chromium.chrome.shell',
    'native_lib_version_name': '<(version_full)',
    'package_name': 'chrome_shell_apk',
  },
  'targets': [
    {
      'target_name': 'libchromeshell',
      'type': 'shared_library',
      'dependencies': [
        '../base/base.gyp:base',
        'chrome_android_core',
        'chrome.gyp:browser_ui',
        '../content/content.gyp:content_app_browser',
      ],
      'sources': [
        # This file must always be included in the shared_library step to ensure
        # JNI_OnLoad is exported.
        'app/android/chrome_jni_onload.cc',
        'android/shell/chrome_main_delegate_chrome_shell_android.cc',
        'android/shell/chrome_main_delegate_chrome_shell_android.h',
        "android/shell/chrome_shell_google_location_settings_helper.cc",
        "android/shell/chrome_shell_google_location_settings_helper.h",
      ],
      'include_dirs': [
        '../skia/config',
      ],
      'conditions': [
        [ 'order_profiling!=0', {
          'conditions': [
            [ 'OS=="android"', {
              'dependencies': [ '../tools/cygprofile/cygprofile.gyp:cygprofile', ],
            }],
          ],
        }],
        [ 'use_allocator!="none"', {
          'dependencies': [
            '../base/allocator/allocator.gyp:allocator', ],
        }],
        ['OS=="android"', {
          'ldflags': [
            # Some android targets still depend on --gc-sections to link.
            # TODO: remove --gc-sections for Debug builds (crbug.com/159847).
            '-Wl,--gc-sections',
          ],
        }],
        ['OS == "android" and clang == 1 and clang_use_lto != 0', {
          'ldflags' : [ '-flto' ]
        }],
      ],
    },
    {
      'target_name': 'chrome_shell_apk',
      'type': 'none',
      'dependencies': [
        'chrome_java',
        'chrome_shell_paks',
        'libchromeshell',
        '../media/media.gyp:media_java',
        '<@(libnetxt_dependencies)',
      ],
      'variables': {
        'java_in_dir': 'android/shell/java',
        'resource_dir': 'android/shell/res',
        'asset_location': '<(PRODUCT_DIR)/../assets/<(package_name)',
        'native_lib_target': 'libchromeshell',
        'additional_native_libs': [
              '<@(libnetxt_native_libs)',],
        'additional_input_paths': [
          '<@(chrome_android_pak_output_resources)',
        ],
      },
      'includes': [ '../build/java_apk.gypi', ],
    },
    {
      # chrome_shell_apk creates a .jar as a side effect. Any java targets
      # that need that .jar in their classpath should depend on this target,
      # chrome_shell_apk_java. Dependents of chrome_shell_apk receive its
      # jar path in the variable 'apk_output_jar_path'.
      # This target should only be used by targets which instrument
      # chrome_shell_apk.
      'target_name': 'chrome_shell_apk_java',
      'type': 'none',
      'dependencies': [
        'chrome_shell_apk',
      ],
      'includes': [ '../build/apk_fake_jar.gypi' ],
    },
    {
      'target_name': 'chrome_shell_paks',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/chrome/chrome_resources.gyp:packed_resources',
        '<(DEPTH)/chrome/chrome_resources.gyp:packed_extra_resources',
      ],
      'copies': [
        {
          'destination': '<(chrome_android_pak_output_folder)',
          'files': [
            '<@(chrome_android_pak_input_resources)',
          ],
        }
      ],
    },
  ],

}
