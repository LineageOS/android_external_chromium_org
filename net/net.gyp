# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'net',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../sdch/sdch.gyp:sdch',
        '../third_party/bzip2/bzip2.gyp:bzip2',
        '../third_party/icu38/icu38.gyp:icui18n',
        '../third_party/icu38/icu38.gyp:icuuc',
        '../third_party/modp_b64/modp_b64.gyp:modp_b64',
        '../third_party/zlib/zlib.gyp:zlib',
      ],
      'sources': [
        'base/address_list.cc',
        'base/address_list.h',
        'base/auth.h',
        'base/base64.cc',
        'base/base64.h',
        'base/bzip2_filter.cc',
        'base/bzip2_filter.h',
        'base/cert_status_flags.cc',
        'base/cert_status_flags.h',
        'base/cert_verifier.cc',
        'base/cert_verifier.h',
        'base/cert_verify_result.h',
        'base/client_socket.cc',
        'base/client_socket.h',
        'base/client_socket_factory.cc',
        'base/client_socket_factory.h',
        'base/client_socket_handle.cc',
        'base/client_socket_handle.h',
        'base/client_socket_pool.cc',
        'base/client_socket_pool.h',
        'base/completion_callback.h',
        'base/connection_type_histograms.cc',
        'base/connection_type_histograms.h',
        'base/cookie_monster.cc',
        'base/cookie_monster.h',
        'base/cookie_policy.cc',
        'base/cookie_policy.h',
        'base/data_url.cc',
        'base/data_url.h',
        'base/directory_lister.cc',
        'base/directory_lister.h',
        'base/dns_resolution_observer.cc',
        'base/dns_resolution_observer.h',
        'base/effective_tld_names.cc',
        'base/effective_tld_names.dat',
        'base/escape.cc',
        'base/escape.h',
        'base/ev_root_ca_metadata.cc',
        'base/ev_root_ca_metadata.h',
        'base/file_stream.h',
        'base/file_stream_posix.cc',
        'base/file_stream_win.cc',
        'base/filter.cc',
        'base/filter.h',
        'base/gzip_filter.cc',
        'base/gzip_filter.h',
        'base/gzip_header.cc',
        'base/gzip_header.h',
        'base/host_resolver.cc',
        'base/host_resolver.h',
        'base/io_buffer.cc',
        'base/io_buffer.h',
        'base/listen_socket.cc',
        'base/listen_socket.h',
        'base/load_flags.h',
        'base/mime_sniffer.cc',
        'base/mime_sniffer.h',
        'base/mime_util.cc',
        'base/mime_util.h',
        'base/net_error_list.h',
        'base/net_errors.cc',
        'base/net_errors.h',
        'base/net_module.cc',
        'base/net_module.h',
        'base/net_resources.h',
        'base/net_util.cc',
        'base/net_util.h',
        'base/net_util_posix.cc',
        'base/net_util_win.cc',
        'base/nss_memio.c',
        'base/platform_mime_util.h',
        # TODO(tc): gnome-vfs? xdgmime? /etc/mime.types?
        'base/platform_mime_util_linux.cc',
        'base/platform_mime_util_mac.cc',
        'base/platform_mime_util_win.cc',
        'base/registry_controlled_domain.cc',
        'base/registry_controlled_domain.h',
        'base/scoped_cert_chain_context.h',
        'base/sdch_filter.cc',
        'base/sdch_filter.h',
        'base/sdch_manager.cc',
        'base/sdch_manager.h',
        'base/socket.h',
        'base/ssl_client_socket.h',
        'base/ssl_client_socket_mac.cc',
        'base/ssl_client_socket_nss.cc',
        'base/ssl_client_socket_win.cc',
        'base/ssl_client_socket_win.h',
        'base/ssl_config_service.cc',
        'base/ssl_config_service.h',
        'base/ssl_info.h',
        'base/ssl_test_util.cc',
        'base/tcp_client_socket.h',
        'base/tcp_client_socket_libevent.cc',
        'base/tcp_client_socket_win.cc',
        'base/telnet_server.cc',
        'base/telnet_server.h',
        'base/upload_data.cc',
        'base/upload_data.h',
        'base/upload_data_stream.cc',
        'base/upload_data_stream.h',
        'base/wininet_util.cc',
        'base/wininet_util.h',
        'base/winsock_init.cc',
        'base/winsock_init.h',
        'base/x509_certificate.cc',
        'base/x509_certificate.h',
        'base/x509_certificate_mac.cc',
        'base/x509_certificate_nss.cc',
        'base/x509_certificate_win.cc',
        'build/precompiled_net.cc',
        'build/precompiled_net.h',
        'disk_cache/addr.cc',
        'disk_cache/addr.h',
        'disk_cache/backend_impl.cc',
        'disk_cache/backend_impl.h',
        'disk_cache/block_files.cc',
        'disk_cache/block_files.h',
        'disk_cache/cache_util.h',
        'disk_cache/cache_util_posix.cc',
        'disk_cache/cache_util_win.cc',
        'disk_cache/disk_cache.h',
        'disk_cache/disk_format.h',
        'disk_cache/entry_impl.cc',
        'disk_cache/entry_impl.h',
        'disk_cache/errors.h',
        'disk_cache/eviction.cc',
        'disk_cache/eviction.h',
        'disk_cache/file.h',
        'disk_cache/file_block.h',
        'disk_cache/file_lock.cc',
        'disk_cache/file_lock.h',
        'disk_cache/file_posix.cc',
        'disk_cache/file_win.cc',
        'disk_cache/hash.cc',
        'disk_cache/hash.h',
        'disk_cache/mapped_file.h',
        'disk_cache/mapped_file_posix.cc',
        'disk_cache/mapped_file_win.cc',
        'disk_cache/mem_backend_impl.cc',
        'disk_cache/mem_backend_impl.h',
        'disk_cache/mem_entry_impl.cc',
        'disk_cache/mem_entry_impl.h',
        'disk_cache/mem_rankings.cc',
        'disk_cache/mem_rankings.h',
        'disk_cache/rankings.cc',
        'disk_cache/rankings.h',
        'disk_cache/stats.cc',
        'disk_cache/stats.h',
        'disk_cache/stats_histogram.cc',
        'disk_cache/stats_histogram.h',
        'disk_cache/storage_block-inl.h',
        'disk_cache/storage_block.h',
        'disk_cache/trace.cc',
        'disk_cache/trace.h',
        'ftp/ftp_auth_cache.cc',
        'ftp/ftp_auth_cache.h',
        'ftp/ftp_network_layer.cc',
        'ftp/ftp_network_layer.h',
        'ftp/ftp_network_session.h',
        'ftp/ftp_network_transaction.cc',
        'ftp/ftp_network_transaction.h',
        'ftp/ftp_request_info.h',
        'ftp/ftp_response_info.h',
        'ftp/ftp_transaction.h',
        'ftp/ftp_transaction_factory.h',
        'http/http_atom_list.h',
        'http/http_auth.cc',
        'http/http_auth.h',
        'http/http_auth_cache.cc',
        'http/http_auth_cache.h',
        'http/http_auth_handler.h',
        'http/http_auth_handler.cc',
        'http/http_auth_handler_basic.cc',
        'http/http_auth_handler_basic.h',
        'http/http_auth_handler_digest.cc',
        'http/http_auth_handler_digest.h',
        'http/http_cache.cc',
        'http/http_cache.h',
        'http/http_chunked_decoder.cc',
        'http/http_chunked_decoder.h',
        'http/http_network_layer.cc',
        'http/http_network_layer.h',
        'http/http_network_session.h',
        'http/http_network_transaction.cc',
        'http/http_network_transaction.h',
        'http/http_request_info.h',
        'http/http_response_headers.cc',
        'http/http_response_headers.h',
        'http/http_response_info.cc',
        'http/http_response_info.h',
        'http/http_transaction.h',
        'http/http_transaction_factory.h',
        'http/http_util.cc',
        'http/http_util.h',
        'http/http_vary_data.cc',
        'http/http_vary_data.h',
        'proxy/proxy_config_service_fixed.h',
        'proxy/proxy_config_service_win.cc',
        'proxy/proxy_config_service_win.h',
        'proxy/proxy_resolver_mac.cc',
        'proxy/proxy_resolver_winhttp.cc',
        'proxy/proxy_resolver_winhttp.h',
        'proxy/proxy_script_fetcher.cc',
        'proxy/proxy_script_fetcher.h',
        'proxy/proxy_server.cc',
        'proxy/proxy_server.h',
        'proxy/proxy_service.cc',
        'proxy/proxy_service.h',
        'url_request/mime_sniffer_proxy.cc',
        'url_request/mime_sniffer_proxy.h',
        'url_request/url_request.cc',
        'url_request/url_request.h',
        'url_request/url_request_about_job.cc',
        'url_request/url_request_about_job.h',
        'url_request/url_request_context.h',
        'url_request/url_request_error_job.cc',
        'url_request/url_request_error_job.h',
        'url_request/url_request_file_dir_job.cc',
        'url_request/url_request_file_dir_job.h',
        'url_request/url_request_file_job.cc',
        'url_request/url_request_file_job.h',
        'url_request/url_request_filter.cc',
        'url_request/url_request_filter.h',
        'url_request/url_request_ftp_job.cc',
        'url_request/url_request_ftp_job.h',
        'url_request/url_request_http_job.cc',
        'url_request/url_request_http_job.h',
        'url_request/url_request_inet_job.cc',
        'url_request/url_request_inet_job.h',
        'url_request/url_request_job.cc',
        'url_request/url_request_job.h',
        'url_request/url_request_job_manager.cc',
        'url_request/url_request_job_manager.h',
        'url_request/url_request_job_metrics.cc',
        'url_request/url_request_job_metrics.h',
        'url_request/url_request_job_tracker.cc',
        'url_request/url_request_job_tracker.h',
        'url_request/url_request_simple_job.cc',
        'url_request/url_request_simple_job.h',
        'url_request/url_request_status.h',
        'url_request/url_request_test_job.cc',
        'url_request/url_request_test_job.h',
        'url_request/url_request_view_cache_job.cc',
        'url_request/url_request_view_cache_job.h',
      ],
      'sources!': [
        'build/precompiled_net.h',
        'build/precompiled_net.cc',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
      ],
      'conditions': [
        [ 'OS == "win"', {
            'sources/': [ ['exclude', '_(mac|linux|posix)\\.cc$'] ],
            'sources!': [
              'base/tcp_client_socket_libevent.cc',
            ],
            'dependencies': [
              'net_resources',
              'tld_cleanup',
            ],
            'configurations': {
              'Debug': {
                'msvs_precompiled_header': 'build/precompiled_net.h',
                'msvs_precompiled_source': 'build/precompiled_net.cc',
              },
            },
          },
          {  # else: OS != "win"
            'sources!': [
              'base/ssl_config_service.cc',
              'base/wininet_util.cc',
              'base/winsock_init.cc',
              'proxy/proxy_resolver_winhttp.cc',
              'url_request/url_request_ftp_job.cc',
              'url_request/url_request_inet_job.cc',
            ],
          },
        ],
        [ 'OS == "linux"', {
            'sources/': [ ['exclude', '_(mac|win)\\.cc$'] ],
            'dependencies': [
              'net_resources',
            ],
          },
          {  # else: OS != "linux"
            'sources!': [
              'base/nss_memio.c',
              'base/ssl_client_socket_nss.cc',
              'base/x509_certificate_nss.cc',
            ],
            # Get U_STATIC_IMPLEMENTATION and -I directories on Linux.
            'dependencies': [
              '../third_party/icu38/icu38.gyp:icui18n',
              '../third_party/icu38/icu38.gyp:icuuc',
            ],
          },
        ],
        [ 'OS == "mac"', {
            'sources/': [ ['exclude', '_(linux|win)\\.cc$'] ],
            'link_settings': {
              'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/Security.framework',
                '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
              ]
            },
          },
        ],
        [ 'OS == "win"', {
          # This used to live in build_convert_tld_data.rules
          #'msvs_tool_files': ['build/convert_tld_data.rules'],
          'rules': [
             {
               'rule_name': 'tld_convert',
               'extension': 'dat',
               'inputs': [ '<(RULE_INPUT_PATH)' ],
               'outputs':
                 ['<(INTERMEDIATE_DIR)/../<(RULE_INPUT_ROOT)_clean.dat'],
               'action':
                 ['<(PRODUCT_DIR)/tld_cleanup', '<@(_inputs)', '<@(_outputs)'],
              },
            ],
        },],
      ],
    },
    {
      'target_name': 'net_unittests',
      'type': 'executable',
      'dependencies': [
        'net',
        '../base/base.gyp:base',
        '../testing/gtest.gyp:gtest',
      ],
      'sources': [
        'base/base64_unittest.cc',
        'base/bzip2_filter_unittest.cc',
        'base/client_socket_pool_unittest.cc',
        'base/cookie_monster_unittest.cc',
        'base/cookie_policy_unittest.cc',
        'base/data_url_unittest.cc',
        'base/directory_lister_unittest.cc',
        'base/escape_unittest.cc',
        'base/file_stream_unittest.cc',
        'base/filter_unittest.cc',
        'base/gzip_filter_unittest.cc',
        'base/host_resolver_unittest.cc',
        'base/listen_socket_unittest.cc',
        'base/listen_socket_unittest.h',
        'base/mime_sniffer_unittest.cc',
        'base/mime_util_unittest.cc',
        'base/net_util_unittest.cc',
        'base/registry_controlled_domain_unittest.cc',
        'base/run_all_unittests.cc',
        'base/sdch_filter_unittest.cc',
        'base/ssl_client_socket_unittest.cc',
        'base/ssl_config_service_unittest.cc',
        'base/tcp_client_socket_unittest.cc',
        'base/telnet_server_unittest.cc',
        'base/test_completion_callback_unittest.cc',
        'base/wininet_util_unittest.cc',
        'base/x509_certificate_unittest.cc',
        'disk_cache/addr_unittest.cc',
        'disk_cache/backend_unittest.cc',
        'disk_cache/block_files_unittest.cc',
        'disk_cache/disk_cache_test_base.cc',
        'disk_cache/disk_cache_test_base.h',
        'disk_cache/disk_cache_test_util.cc',
        'disk_cache/disk_cache_test_util.h',
        'disk_cache/entry_unittest.cc',
        'disk_cache/mapped_file_unittest.cc',
        'disk_cache/storage_block_unittest.cc',
        'ftp/ftp_auth_cache_unittest.cc',
        'http/http_auth_cache_unittest.cc',
        'http/http_auth_handler_basic_unittest.cc',
        'http/http_auth_handler_digest_unittest.cc',
        'http/http_auth_unittest.cc',
        'http/http_cache_unittest.cc',
        'http/http_chunked_decoder_unittest.cc',
        'http/http_network_layer_unittest.cc',
        'http/http_network_transaction_unittest.cc',
        'http/http_response_headers_unittest.cc',
        'http/http_transaction_unittest.cc',
        'http/http_transaction_unittest.h',
        'http/http_util_unittest.cc',
        'http/http_vary_data_unittest.cc',
        'proxy/proxy_script_fetcher_unittest.cc',
        'proxy/proxy_service_unittest.cc',
        'url_request/url_request_unittest.cc',
        'url_request/url_request_unittest.h',
      ],
      'conditions': [
        [ 'OS != "win"', {
            'sources!': [
              'base/wininet_util_unittest.cc',
            ],
          },
        ],
        [ 'OS == "linux"', {
            'sources!': [
              'base/sdch_filter_unittest.cc',
              'base/ssl_config_service_unittest.cc',
            ],
          },
        ],
        [ 'OS == "mac"', {
            'sources!': [
              'base/ssl_config_service_unittest.cc',
            ],
          },
        ],
        # This is needed to trigger the dll copy step on windows.
        # TODO(mark): Specifying this here shouldn't be necessary.
        [ 'OS == "win"', {
            'dependencies': [
              '../third_party/icu38/icu38.gyp:icudata',
            ],
          },
        ],
      ],
    },
    {
      'target_name': 'net_perftests',
      'type': 'executable',
      'dependencies': [
        'net',
        '../base/base.gyp:base',
        '../testing/gtest.gyp:gtest',
      ],
      'sources': [
        '../base/perftimer.cc',
        '../base/run_all_perftests.cc',
        'base/cookie_monster_perftest.cc',
        'disk_cache/disk_cache_perftest.cc',
        'disk_cache/disk_cache_test_util.cc',
      ],
      'conditions': [
        # This is needed to trigger the dll copy step on windows.
        # TODO(mark): Specifying this here shouldn't be necessary.
        [ 'OS == "win"', {
            'dependencies': [
              '../third_party/icu38/icu38.gyp:icudata',
            ],
          },
        ],
      ],
    },
    {
      'target_name': 'stress_cache',
      'type': 'executable',
      'dependencies': [
        'net',
        '../base/base.gyp:base',
      ],
      'sources': [
        'disk_cache/disk_cache_test_util.cc',
        'disk_cache/stress_cache.cc',
      ],
    },
    {
      'target_name': 'tld_cleanup',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:base',
        '../build/temp_gyp/googleurl.gyp:googleurl',
      ],
      'sources': [
        'tools/tld_cleanup/tld_cleanup.cc',
      ],
    },
    {
      'target_name': 'crash_cache',
      'type': 'executable',
      'dependencies': [
        'net',
        '../base/base.gyp:base',
      ],
      'sources': [
        'tools/crash_cache/crash_cache.cc',
        'disk_cache/disk_cache_test_util.cc',
      ],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'net_resources',
          'type': 'none',
          'sources': [
            'base/net_resources.grd',
          ],
          #'msvs_tool_files': ['../tools/grit/build/grit_resources.rules'],
          # This was orignally in grit_resources.rules
          # NOTE: this version doesn't mimic the Properties specified there.
          'rules': [
            {
              'rule_name': 'grit',
              'extension': 'grd',
              'inputs': [
                '<(DEPTH)/tools/grit/grit.py',
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/grit_derived_sources/<(RULE_INPUT_ROOT).h',
              ],
              'action':
                ['python', '<(DEPTH)/tools/grit/grit.py', '-i', '<(RULE_INPUT_PATH)', 'build', '-o', '<(SHARED_INTERMEDIATE_DIR)/grit_derived_sources'],
            },
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '<(SHARED_INTERMEDIATE_DIR)/grit_derived_sources',
            ],
          },
        },
        {
          # TODO(port): dump_cache is still Windows-specific.
          'target_name': 'dump_cache',
          'type': 'executable',
          'dependencies': [
            'net',
            '../base/base.gyp:base',
          ],
          'sources': [
            'tools/dump_cache/dump_cache.cc',
            'tools/dump_cache/dump_files.cc',
            'tools/dump_cache/upgrade.cc',
          ],
        },
      ],
    }],
    ['OS=="linux"', {
      'targets': [
        {
          'target_name': 'net_resources',
          'type': 'resource',
          'sources': [
            'base/net_resources.grd',
            '../../grit_derived_sources/effective_tld_names_clean.dat',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '../../grit_derived_sources'
              # FIXME: Should use one of the INTERMEDIATE dirs, e.g.:
              # '$(obj)/gen'
            ],
          },
          'actions': [
            {
              'action_name': 'net_resources_h',
              'inputs': [
                'tld_cleanup',
                'base/effective_tld_names.dat',
              ],
              'outputs': [
                '../../grit_derived_sources/effective_tld_names_clean.dat',
              ],
              # An 'action' like this would expand things at gyp time:
              #'action': 'tld_cleanup <@(_inputs) <@(_outputs)',
              # But that doesn't work well with the SCons variant dir
              # stuff that builds everything underneath Hammer.  Just
              # put a SCons string in the action, at least for now.
              'action': ['${SOURCES[0]}', '${SOURCES[1]}', '$TARGET'],
            }
          ],
        },
      ],
    }],
  ],
}
