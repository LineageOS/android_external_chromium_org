# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'webkit_storage_common',
      'type': '<(component)',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/third_party/WebKit/public/blink.gyp:blink',
        '<(DEPTH)/webkit/base/webkit_base.gyp:webkit_base',
      ],
      'defines': ['WEBKIT_STORAGE_COMMON_IMPLEMENTATION'],
      'sources': [
        'common/webkit_storage_common_export.h',
        'common/appcache/appcache_interfaces.cc',
        'common/appcache/appcache_interfaces.h',
        'common/blob/blob_data.cc',
        'common/blob/blob_data.h',
        'common/blob/scoped_file.cc',
        'common/blob/scoped_file.h',
        'common/blob/shareable_file_reference.cc',
        'common/blob/shareable_file_reference.h',
        'common/database/database_connections.cc',
        'common/database/database_connections.h',
        'common/dom_storage/dom_storage_map.cc',
        'common/dom_storage/dom_storage_map.h',
        'common/dom_storage/dom_storage_types.cc',
        'common/dom_storage/dom_storage_types.h',
        'common/fileapi/directory_entry.h',
        'common/fileapi/file_system_types.h',
        'common/fileapi/file_system_util.cc',
        'common/fileapi/file_system_util.h',
        'common/quota/quota_status_code.cc',
        'common/quota/quota_status_code.h',
        'common/quota/quota_types.h',
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ],
}
