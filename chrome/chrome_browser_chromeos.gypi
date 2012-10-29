# Copyright (c) 2012 The Chromium Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'browser_chromeos',
      'type': 'static_library',
      'variables': {
        'conditions': [
          ['sysroot!=""', {
            'pkg-config': '../build/linux/pkg-config-wrapper "<(sysroot)" "<(target_arch)"',
          }, {
            'pkg-config': 'pkg-config'
          }],
        ],
        # Override to dynamically link the cras (ChromeOS audio) library.
        'use_cras%': 0,
        'enable_wexit_time_destructors': 1,
      },
      'dependencies': [
        # TODO(tbarzic): Cleanup this list.
        'app/policy/cloud_policy_codegen.gyp:policy',
        'autofill_regexes',
        'browser_extensions',
        'browser/chromeos/input_method/input_method.gyp:gencode',
        'browser/performance_monitor/performance_monitor.gyp:performance_monitor',
        'cert_logger_proto',
        'chrome_resources.gyp:chrome_extra_resources',
        'chrome_resources.gyp:chrome_resources',
        'chrome_resources.gyp:platform_locale_settings',
        'chrome_resources.gyp:theme_resources',
        'common',
        'common/extensions/api/api.gyp:api',
        'common_net',
        'contacts_proto',
        'debugger',
        'drive_proto',
        'feedback_proto',
        'in_memory_url_index_cache_proto',
        'installer_util',
        'safe_browsing_proto',
        'safe_browsing_report_proto',
        'variations_seed_proto',
        '../build/linux/system.gyp:dbus',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../chromeos/chromeos.gyp:chromeos',
        '../content/content.gyp:content_browser',
        '../content/content.gyp:content_common',
        '../crypto/crypto.gyp:crypto',
        '../dbus/dbus.gyp:dbus',
        '../device/device.gyp:device_bluetooth',
        '../media/media.gyp:media',
        '../net/net.gyp:net',
        '../ppapi/ppapi_internal.gyp:ppapi_ipc',  # For PpapiMsg_LoadPlugin
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../sync/protocol/sync_proto.gyp:sync_proto',
        # TODO(akalin): Depend only on syncapi_service from sync.
        '../sync/sync.gyp:syncapi_core',
        '../sync/sync.gyp:syncapi_service',
        '../sync/sync.gyp:sync_notifier',
        '../third_party/adobe/flash/flash_player.gyp:flapper_version_h',
        '../third_party/bzip2/bzip2.gyp:bzip2',
        '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation',
        '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_proto_cpp',
        '../third_party/cld/cld.gyp:cld',
        '../third_party/expat/expat.gyp:expat',
        '../third_party/hunspell/hunspell.gyp:hunspell',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '../third_party/libevent/libevent.gyp:libevent',
        '../third_party/libjingle/libjingle.gyp:libjingle',
        '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber',
        '../third_party/libusb/libusb.gyp:libusb',
        '../third_party/libxml/libxml.gyp:libxml',
        '../third_party/mozc/chrome/chromeos/renderer/chromeos_renderer.gyp:mozc_candidates_proto',
        '../third_party/npapi/npapi.gyp:npapi',
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
        '../third_party/protobuf/protobuf.gyp:protoc#host',
        'chrome_resources.gyp:chrome_strings',
        '../third_party/re2/re2.gyp:re2',
        '../third_party/zlib/zlib.gyp:zlib',
        '../ui/base/strings/ui_strings.gyp:ui_strings',
        '../ui/surface/surface.gyp:surface',
        '../ui/ui.gyp:ui',
        '../ui/ui.gyp:ui_resources',
        '../ui/web_dialogs/web_dialogs.gyp:web_dialogs',
        '../v8/tools/gyp/v8.gyp:v8',
        '../ui/views/controls/webview/webview.gyp:webview',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:user_agent',
        '../webkit/support/webkit_support.gyp:webkit_resources',
        '../webkit/support/webkit_support.gyp:webkit_storage',
      ],
      'defines': [
        '<@(nacl_defines)',
      ],
      'direct_dependent_settings': {
        'defines': [
          '<@(nacl_defines)',
        ],
      },
      'export_dependent_settings': [
        '../sync/sync.gyp:sync_notifier',
      ],
      'sources': [
        # All .cc, .h, .m, and .mm files unde browser/chromeos, except for tests
        # and mocks.
        'browser/chromeos/accessibility/accessibility_util.cc',
        'browser/chromeos/accessibility/accessibility_util.h',
        'browser/chromeos/audio/audio_handler.cc',
        'browser/chromeos/audio/audio_handler.h',
        'browser/chromeos/audio/audio_mixer.h',
        'browser/chromeos/audio/audio_mixer_alsa.cc',
        'browser/chromeos/audio/audio_mixer_alsa.h',
        'browser/chromeos/audio/audio_mixer_cras.cc',
        'browser/chromeos/audio/audio_mixer_cras.h',
        'browser/chromeos/background/ash_user_wallpaper_delegate.cc',
        'browser/chromeos/background/ash_user_wallpaper_delegate.h',
        'browser/chromeos/boot_times_loader.cc',
        'browser/chromeos/boot_times_loader.h',
        'browser/chromeos/choose_mobile_network_dialog.cc',
        'browser/chromeos/choose_mobile_network_dialog.h',
        'browser/chromeos/chrome_browser_main_chromeos.cc',
        'browser/chromeos/chrome_browser_main_chromeos.h',
        'browser/chromeos/contacts/contact_database.cc',
        'browser/chromeos/contacts/contact_database.h',
        'browser/chromeos/contacts/contact_manager.cc',
        'browser/chromeos/contacts/contact_manager.h',
        'browser/chromeos/contacts/contact_manager_observer.h',
        'browser/chromeos/contacts/contact_map.cc',
        'browser/chromeos/contacts/contact_map.h',
        'browser/chromeos/contacts/contact_store.h',
        'browser/chromeos/contacts/contact_store_observer.h',
        'browser/chromeos/contacts/gdata_contacts_service.cc',
        'browser/chromeos/contacts/gdata_contacts_service.h',
        'browser/chromeos/contacts/google_contact_store.cc',
        'browser/chromeos/contacts/google_contact_store.h',
        'browser/chromeos/cros/burn_library.cc',
        'browser/chromeos/cros/burn_library.h',
        'browser/chromeos/cros/cellular_data_plan.cc',
        'browser/chromeos/cros/cellular_data_plan.h',
        'browser/chromeos/cros/cert_library.cc',
        'browser/chromeos/cros/cert_library.h',
        'browser/chromeos/cros/certificate_pattern.cc',
        'browser/chromeos/cros/certificate_pattern.h',
        'browser/chromeos/cros/cros_library.cc',
        'browser/chromeos/cros/cros_library.h',
        'browser/chromeos/cros/cros_network_functions.cc',
        'browser/chromeos/cros/cros_network_functions.h',
        'browser/chromeos/cros/cryptohome_library.cc',
        'browser/chromeos/cros/cryptohome_library.h',
        'browser/chromeos/cros/enum_mapper.h',
        'browser/chromeos/cros/native_network_constants.cc',
        'browser/chromeos/cros/native_network_constants.h',
        'browser/chromeos/cros/native_network_parser.cc',
        'browser/chromeos/cros/native_network_parser.h',
        'browser/chromeos/cros/network_constants.h',
        'browser/chromeos/cros/network_ip_config.cc',
        'browser/chromeos/cros/network_ip_config.h',
        'browser/chromeos/cros/network_library.cc',
        'browser/chromeos/cros/network_library.h',
        'browser/chromeos/cros/network_library_impl_base.cc',
        'browser/chromeos/cros/network_library_impl_base.h',
        'browser/chromeos/cros/network_library_impl_cros.cc',
        'browser/chromeos/cros/network_library_impl_cros.h',
        'browser/chromeos/cros/network_library_impl_stub.cc',
        'browser/chromeos/cros/network_library_impl_stub.h',
        'browser/chromeos/cros/network_parser.cc',
        'browser/chromeos/cros/network_parser.h',
        'browser/chromeos/cros/network_ui_data.cc',
        'browser/chromeos/cros/network_ui_data.h',
        'browser/chromeos/cros/onc_constants.cc',
        'browser/chromeos/cros/onc_constants.h',
        'browser/chromeos/cros/onc_network_parser.cc',
        'browser/chromeos/cros/onc_network_parser.h',
        'browser/chromeos/cros/sms_watcher.cc',
        'browser/chromeos/cros/sms_watcher.h',
        'browser/chromeos/customization_document.cc',
        'browser/chromeos/customization_document.h',
        'browser/chromeos/display/display_preferences.cc',
        'browser/chromeos/display/display_preferences.h',
        'browser/chromeos/display/overscan_calibrator.cc',
        'browser/chromeos/display/overscan_calibrator.h',
        'browser/chromeos/display/primary_display_switch_observer.cc',
        'browser/chromeos/display/primary_display_switch_observer.h',
        'browser/chromeos/dbus/cros_dbus_service.cc',
        'browser/chromeos/dbus/cros_dbus_service.h',
        'browser/chromeos/dbus/proxy_resolution_service_provider.cc',
        'browser/chromeos/dbus/proxy_resolution_service_provider.h',
        'browser/chromeos/device_hierarchy_observer.h',
        'browser/chromeos/drive/document_entry_conversion.cc',
        'browser/chromeos/drive/document_entry_conversion.h',
        'browser/chromeos/drive/drive_api_service.cc',
        'browser/chromeos/drive/drive_api_service.h',
        'browser/chromeos/drive/drive_cache.cc',
        'browser/chromeos/drive/drive_cache.h',
        'browser/chromeos/drive/drive_cache_metadata.cc',
        'browser/chromeos/drive/drive_cache_metadata.h',
        'browser/chromeos/drive/drive_cache_observer.h',
        'browser/chromeos/drive/drive_download_observer.cc',
        'browser/chromeos/drive/drive_download_observer.h',
        'browser/chromeos/drive/drive_feed_loader.cc',
        'browser/chromeos/drive/drive_feed_loader.h',
        'browser/chromeos/drive/drive_feed_loader_observer.h',
        'browser/chromeos/drive/drive_feed_processor.cc',
        'browser/chromeos/drive/drive_feed_processor.h',
        'browser/chromeos/drive/drive_file_error.cc',
        'browser/chromeos/drive/drive_file_error.h',
        'browser/chromeos/drive/drive_file_system.cc',
        'browser/chromeos/drive/drive_file_system.h',
        'browser/chromeos/drive/drive_file_system_interface.h',
        'browser/chromeos/drive/drive_file_system_observer.h',
        'browser/chromeos/drive/drive_file_system_proxy.cc',
        'browser/chromeos/drive/drive_file_system_proxy.h',
        'browser/chromeos/drive/drive_file_system_util.cc',
        'browser/chromeos/drive/drive_file_ststem_util.h',
        'browser/chromeos/drive/drive_files.cc',
        'browser/chromeos/drive/drive_files.h',
        'browser/chromeos/drive/drive_prefetcher.cc',
        'browser/chromeos/drive/drive_prefetcher.h',
        'browser/chromeos/drive/drive_protocol_handler.cc',
        'browser/chromeos/drive/drive_protocol_handler.h',
        'browser/chromeos/drive/drive_resource_metadata.cc',
        'browser/chromeos/drive/drive_resource_metadata.h',
        'browser/chromeos/drive/drive_scheduler.cc',
        'browser/chromeos/drive/drive_scheduler.h',
        'browser/chromeos/drive/drive_sync_client.cc',
        'browser/chromeos/drive/drive_sync_client.h',
        'browser/chromeos/drive/drive_sync_client_observer.h',
        'browser/chromeos/drive/drive_system_service.cc',
        'browser/chromeos/drive/drive_system_service.h',
        'browser/chromeos/drive/drive_task_executor.cc',
        'browser/chromeos/drive/drive_task_executor.h',
        'browser/chromeos/drive/drive_webapps_registry.cc',
        'browser/chromeos/drive/drive_webapps_registry.h',
        'browser/chromeos/drive/file_change.cc',
        'browser/chromeos/drive/file_change.h',
        'browser/chromeos/drive/file_system/copy_operation.cc',
        'browser/chromeos/drive/file_system/copy_operation.h',
        'browser/chromeos/drive/file_system/drive_operations.cc',
        'browser/chromeos/drive/file_system/drive_operations.h',
        'browser/chromeos/drive/file_system/move_operation.cc',
        'browser/chromeos/drive/file_system/move_operation.h',
        'browser/chromeos/drive/file_system/remove_operation.cc',
        'browser/chromeos/drive/file_system/remove_operation.h',
        'browser/chromeos/drive/file_system/operation_observer.h',
        'browser/chromeos/drive/file_write_helper.cc',
        'browser/chromeos/drive/file_write_helper.h',
        'browser/chromeos/drive/stale_cache_files_remover.cc',
        'browser/chromeos/drive/stale_cache_files_remover.h',
        'browser/chromeos/enrollment_dialog_view.cc',
        'browser/chromeos/enrollment_dialog_view.h',
        'browser/chromeos/enterprise_extension_observer.cc',
        'browser/chromeos/enterprise_extension_observer.h',
        'browser/chromeos/extensions/default_app_order.cc',
        'browser/chromeos/extensions/default_app_order.h',
        'browser/chromeos/extensions/echo_private_api.cc',
        'browser/chromeos/extensions/echo_private_api.h',
        'browser/chromeos/extensions/file_browser_event_router.cc',
        'browser/chromeos/extensions/file_browser_event_router.h',
        'browser/chromeos/extensions/file_browser_notifications.cc',
        'browser/chromeos/extensions/file_browser_notifications.h',
        'browser/chromeos/extensions/file_handler_util.cc',
        'browser/chromeos/extensions/file_handler_util.h',
        'browser/chromeos/extensions/file_manager_util.cc',
        'browser/chromeos/extensions/file_manager_util.h',
        'browser/chromeos/extensions/info_private_api.cc',
        'browser/chromeos/extensions/info_private_api.h',
        'browser/chromeos/extensions/input_method_event_router.cc',
        'browser/chromeos/extensions/input_method_event_router.h',
        'browser/chromeos/extensions/media_player_event_router.cc',
        'browser/chromeos/extensions/media_player_event_router.h',
        'browser/chromeos/extensions/power/power_api.cc',
        'browser/chromeos/extensions/power/power_api.h',
        'browser/chromeos/extensions/power/power_api_manager.cc',
        'browser/chromeos/extensions/power/power_api_manager.h',
        'browser/chromeos/external_metrics.cc',
        'browser/chromeos/external_metrics.h',
        'browser/chromeos/external_protocol_dialog.cc',
        'browser/chromeos/external_protocol_dialog.h',
        'browser/chromeos/gview_request_interceptor.cc',
        'browser/chromeos/gview_request_interceptor.h',
        'browser/chromeos/imageburner/burn_controller.cc',
        'browser/chromeos/imageburner/burn_controller.h',
        'browser/chromeos/imageburner/burn_manager.cc',
        'browser/chromeos/imageburner/burn_manager.h',
        'browser/chromeos/input_method/browser_state_monitor.cc',
        'browser/chromeos/input_method/browser_state_monitor.h',
        'browser/chromeos/input_method/candidate_window.cc',
        'browser/chromeos/input_method/candidate_window.h',
        'browser/chromeos/input_method/ibus_controller.cc',
        'browser/chromeos/input_method/ibus_controller.h',
        'browser/chromeos/input_method/ibus_controller_base.cc',
        'browser/chromeos/input_method/ibus_controller_base.h',
        'browser/chromeos/input_method/ibus_controller_impl.cc',
        'browser/chromeos/input_method/ibus_controller_impl.h',
        'browser/chromeos/input_method/input_method_engine_ibus.cc',
        'browser/chromeos/input_method/input_method_engine_ibus.h',
        'browser/chromeos/input_method/ibus_keymap.cc',
        'browser/chromeos/input_method/ibus_keymap.h',
        'browser/chromeos/input_method/ibus_ui_controller.cc',
        'browser/chromeos/input_method/ibus_ui_controller.h',
        'browser/chromeos/input_method/input_method_config.cc',
        'browser/chromeos/input_method/input_method_config.h',
        'browser/chromeos/input_method/input_method_descriptor.cc',
        'browser/chromeos/input_method/input_method_descriptor.h',
        'browser/chromeos/input_method/input_method_engine.cc',
        'browser/chromeos/input_method/input_method_engine.h',
        'browser/chromeos/input_method/input_method_manager.cc',
        'browser/chromeos/input_method/input_method_manager.h',
        'browser/chromeos/input_method/input_method_manager_impl.cc',
        'browser/chromeos/input_method/input_method_manager_impl.h',
        'browser/chromeos/input_method/input_method_property.cc',
        'browser/chromeos/input_method/input_method_property.h',
        'browser/chromeos/input_method/input_method_util.cc',
        'browser/chromeos/input_method/input_method_util.h',
        'browser/chromeos/input_method/input_method_whitelist.cc',
        'browser/chromeos/input_method/input_method_whitelist.h',
        'browser/chromeos/input_method/mock_ibus_controller.cc',
        'browser/chromeos/input_method/mock_ibus_controller.h',
        'browser/chromeos/input_method/xkeyboard.cc',
        'browser/chromeos/input_method/xkeyboard.h',
        'browser/chromeos/kiosk_mode/kiosk_mode_idle_logout.cc',
        'browser/chromeos/kiosk_mode/kiosk_mode_idle_logout.h',
        'browser/chromeos/kiosk_mode/kiosk_mode_screensaver.cc',
        'browser/chromeos/kiosk_mode/kiosk_mode_screensaver.h',
        'browser/chromeos/kiosk_mode/kiosk_mode_settings.cc',
        'browser/chromeos/kiosk_mode/kiosk_mode_settings.h',
        'browser/chromeos/language_preferences.cc',
        'browser/chromeos/language_preferences.h',
        'browser/chromeos/locale_change_guard.cc',
        'browser/chromeos/locale_change_guard.h',
        'browser/chromeos/login/auth_attempt_state.cc',
        'browser/chromeos/login/auth_attempt_state.h',
        'browser/chromeos/login/auth_attempt_state_resolver.cc',
        'browser/chromeos/login/auth_attempt_state_resolver.h',
        'browser/chromeos/login/authentication_notification_details.h',
        'browser/chromeos/login/authenticator.cc',
        'browser/chromeos/login/authenticator.h',
        'browser/chromeos/login/base_login_display_host.cc',
        'browser/chromeos/login/base_login_display_host.h',
        'browser/chromeos/login/base_login_display_host.h',
        'browser/chromeos/login/camera.cc',
        'browser/chromeos/login/camera.h',
        'browser/chromeos/login/camera_controller.cc',
        'browser/chromeos/login/camera_controller.h',
        'browser/chromeos/login/camera_detector.cc',
        'browser/chromeos/login/camera_detector.h',
        'browser/chromeos/login/captive_portal_view.cc',
        'browser/chromeos/login/captive_portal_view.h',
        'browser/chromeos/login/captive_portal_window_proxy.cc',
        'browser/chromeos/login/captive_portal_window_proxy.h',
        'browser/chromeos/login/default_user_images.cc',
        'browser/chromeos/login/default_user_images.h',
        'browser/chromeos/login/enrollment/enterprise_enrollment_screen.cc',
        'browser/chromeos/login/enrollment/enterprise_enrollment_screen.h',
        'browser/chromeos/login/enrollment/enterprise_enrollment_screen_actor.cc',
        'browser/chromeos/login/enrollment/enterprise_enrollment_screen_actor.h',
        'browser/chromeos/login/eula_screen.cc',
        'browser/chromeos/login/eula_screen.h',
        'browser/chromeos/login/eula_screen_actor.h',
        'browser/chromeos/login/existing_user_controller.cc',
        'browser/chromeos/login/existing_user_controller.h',
        'browser/chromeos/login/help_app_launcher.cc',
        'browser/chromeos/login/help_app_launcher.h',
        'browser/chromeos/login/helper.cc',
        'browser/chromeos/login/helper.h',
        'browser/chromeos/login/html_page_screen.cc',
        'browser/chromeos/login/html_page_screen.h',
        'browser/chromeos/login/language_list.cc',
        'browser/chromeos/login/language_list.h',
        'browser/chromeos/login/language_switch_menu.cc',
        'browser/chromeos/login/language_switch_menu.h',
        'browser/chromeos/login/lock_window.cc',
        'browser/chromeos/login/lock_window.h',
        'browser/chromeos/login/lock_window_aura.cc',
        'browser/chromeos/login/lock_window_aura.h',
        'browser/chromeos/login/login_display.cc',
        'browser/chromeos/login/login_display.h',
        'browser/chromeos/login/login_display_host.h',
        'browser/chromeos/login/login_performer.cc',
        'browser/chromeos/login/login_performer.h',
        'browser/chromeos/login/login_status_consumer.cc',
        'browser/chromeos/login/login_status_consumer.h',
        'browser/chromeos/login/login_utils.cc',
        'browser/chromeos/login/login_utils.h',
        'browser/chromeos/login/login_web_dialog.cc',
        'browser/chromeos/login/login_web_dialog.h',
        'browser/chromeos/login/login_wizard.h',
        'browser/chromeos/login/message_bubble.cc',
        'browser/chromeos/login/message_bubble.h',
        'browser/chromeos/login/network_screen.cc',
        'browser/chromeos/login/network_screen.h',
        'browser/chromeos/login/network_screen_actor.h',
        'browser/chromeos/login/oauth1_token_fetcher.cc',
        'browser/chromeos/login/oauth1_token_fetcher.h',
        'browser/chromeos/login/oauth_login_verifier.cc',
        'browser/chromeos/login/oauth_login_verifier.h',
        'browser/chromeos/login/online_attempt.cc',
        'browser/chromeos/login/online_attempt.h',
        'browser/chromeos/login/online_attempt_host.cc',
        'browser/chromeos/login/online_attempt_host.h',
        'browser/chromeos/login/oobe_display.h',
        'browser/chromeos/login/parallel_authenticator.cc',
        'browser/chromeos/login/parallel_authenticator.h',
        'browser/chromeos/login/password_changed_view.cc',
        'browser/chromeos/login/password_changed_view.h',
        'browser/chromeos/login/policy_oauth_fetcher.cc',
        'browser/chromeos/login/policy_oauth_fetcher.h',
        'browser/chromeos/login/proxy_settings_dialog.cc',
        'browser/chromeos/login/proxy_settings_dialog.h',
        'browser/chromeos/login/registration_screen.cc',
        'browser/chromeos/login/registration_screen.h',
        'browser/chromeos/login/remove_user_delegate.h',
        'browser/chromeos/login/reset_screen.cc',
        'browser/chromeos/login/reset_screen.h',
        'browser/chromeos/login/reset_screen_actor.h',
        'browser/chromeos/login/rounded_rect_painter.cc',
        'browser/chromeos/login/rounded_rect_painter.h',
        'browser/chromeos/login/screen_locker.cc',
        'browser/chromeos/login/screen_locker.h',
        'browser/chromeos/login/screen_locker_delegate.cc',
        'browser/chromeos/login/screen_locker_delegate.h',
        'browser/chromeos/login/screen_observer.h',
        'browser/chromeos/login/simple_jpeg_encoder.cc',
        'browser/chromeos/login/simple_jpeg_encoder.h',
        'browser/chromeos/login/simple_web_view_dialog.cc',
        'browser/chromeos/login/simple_web_view_dialog.h',
        'browser/chromeos/login/take_photo_view.cc',
        'browser/chromeos/login/take_photo_view.h',
        'browser/chromeos/login/test_attempt_state.cc',
        'browser/chromeos/login/test_attempt_state.h',
        'browser/chromeos/login/textfield_with_margin.cc',
        'browser/chromeos/login/textfield_with_margin.h',
        'browser/chromeos/login/tpm_password_fetcher.cc',
        'browser/chromeos/login/tpm_password_fetcher.h',
        'browser/chromeos/login/update_screen.cc',
        'browser/chromeos/login/update_screen.h',
        'browser/chromeos/login/update_screen_actor.h',
        'browser/chromeos/login/user.cc',
        'browser/chromeos/login/user.h',
        'browser/chromeos/login/user_image.cc',
        'browser/chromeos/login/user_image.h',
        'browser/chromeos/login/user_image_loader.cc',
        'browser/chromeos/login/user_image_loader.h',
        'browser/chromeos/login/user_image_manager.h',
        'browser/chromeos/login/user_image_manager.cc',
        'browser/chromeos/login/user_image_manager_impl.cc',
        'browser/chromeos/login/user_image_manager_impl.h',
        'browser/chromeos/login/user_image_screen.cc',
        'browser/chromeos/login/user_image_screen.h',
        'browser/chromeos/login/user_image_screen_actor.h',
        'browser/chromeos/login/user_manager.cc',
        'browser/chromeos/login/user_manager.h',
        'browser/chromeos/login/user_manager_impl.cc',
        'browser/chromeos/login/user_manager_impl.h',
        'browser/chromeos/login/version_info_updater.cc',
        'browser/chromeos/login/version_info_updater.h',
        'browser/chromeos/login/view_screen.h',
        'browser/chromeos/login/wallpaper_manager.cc',
        'browser/chromeos/login/wallpaper_manager.h',
        'browser/chromeos/login/web_page_screen.cc',
        'browser/chromeos/login/web_page_screen.h',
        'browser/chromeos/login/web_page_view.cc',
        'browser/chromeos/login/web_page_view.h',
        'browser/chromeos/login/webui_login_display.cc',
        'browser/chromeos/login/webui_login_display.h',
        'browser/chromeos/login/webui_login_display_host.cc',
        'browser/chromeos/login/webui_login_display_host.h',
        'browser/chromeos/login/webui_login_view.cc',
        'browser/chromeos/login/webui_login_view.h',
        'browser/chromeos/login/webui_screen_locker.cc',
        'browser/chromeos/login/webui_screen_locker.h',
        'browser/chromeos/login/wizard_controller.cc',
        'browser/chromeos/login/wizard_controller.h',
        'browser/chromeos/login/wizard_screen.cc',
        'browser/chromeos/login/wizard_screen.h',
        'browser/chromeos/media/media_player.cc',
        'browser/chromeos/media/media_player.h',
        'browser/chromeos/memory/low_memory_observer.cc',
        'browser/chromeos/memory/low_memory_observer.h',
        'browser/chromeos/memory/oom_priority_manager.cc',
        'browser/chromeos/memory/oom_priority_manager.h',
        'browser/chromeos/mobile/mobile_activator.cc',
        'browser/chromeos/mobile/mobile_activator.h',
        'browser/chromeos/mobile_config.cc',
        'browser/chromeos/mobile_config.h',
        'browser/chromeos/net/cros_network_change_notifier_factory.cc',
        'browser/chromeos/net/cros_network_change_notifier_factory.h',
        'browser/chromeos/net/network_change_notifier_chromeos.cc',
        'browser/chromeos/net/network_change_notifier_chromeos.h',
        'browser/chromeos/network_login_observer.cc',
        'browser/chromeos/network_login_observer.h',
        'browser/chromeos/network_message_observer.cc',
        'browser/chromeos/network_message_observer.h',
        'browser/chromeos/notifications/balloon_view_host_chromeos.cc',
        'browser/chromeos/notifications/balloon_view_host_chromeos.h',
        'browser/chromeos/offline/offline_load_page.cc',
        'browser/chromeos/offline/offline_load_page.h',
        'browser/chromeos/options/network_config_view.cc',
        'browser/chromeos/options/network_config_view.h',
        'browser/chromeos/options/passphrase_textfield.cc',
        'browser/chromeos/options/passphrase_textfield.h',
        'browser/chromeos/options/take_photo_dialog.cc',
        'browser/chromeos/options/take_photo_dialog.h',
        'browser/chromeos/options/vpn_config_view.cc',
        'browser/chromeos/options/vpn_config_view.h',
        'browser/chromeos/options/wifi_config_view.cc',
        'browser/chromeos/options/wifi_config_view.h',
        'browser/chromeos/options/wimax_config_view.cc',
        'browser/chromeos/options/wimax_config_view.h',
        'browser/chromeos/power/brightness_observer.cc',
        'browser/chromeos/power/brightness_observer.h',
        'browser/chromeos/power/output_observer.cc',
        'browser/chromeos/power/output_observer.h',
        'browser/chromeos/power/power_button_observer.cc',
        'browser/chromeos/power/power_button_observer.h',
        'browser/chromeos/power/resume_observer.cc',
        'browser/chromeos/power/resume_observer.h',
        'browser/chromeos/power/screen_dimming_observer.cc',
        'browser/chromeos/power/screen_dimming_observer.h',
        'browser/chromeos/power/screen_lock_observer.cc',
        'browser/chromeos/power/screen_lock_observer.h',
        'browser/chromeos/power/session_state_controller_delegate_chromeos.cc',
        'browser/chromeos/power/session_state_controller_delegate_chromeos.h',
        'browser/chromeos/power/user_activity_notifier.cc',
        'browser/chromeos/power/user_activity_notifier.h',
        'browser/chromeos/power/video_activity_notifier.cc',
        'browser/chromeos/power/video_activity_notifier.h',
        'browser/chromeos/preferences.cc',
        'browser/chromeos/preferences.h',
        'browser/chromeos/prerender_condition_network.cc',
        'browser/chromeos/prerender_condition_network.h',
        'browser/chromeos/process_proxy/process_output_watcher.cc',
        'browser/chromeos/process_proxy/process_output_watcher.h',
        'browser/chromeos/process_proxy/process_proxy.cc',
        'browser/chromeos/process_proxy/process_proxy.h',
        'browser/chromeos/process_proxy/process_proxy_registry.cc',
        'browser/chromeos/process_proxy/process_proxy_registry.h',
        'browser/chromeos/profile_startup.cc',
        'browser/chromeos/profile_startup.h',
        'browser/chromeos/proxy_config_service_impl.cc',
        'browser/chromeos/proxy_config_service_impl.h',
        'browser/chromeos/proxy_cros_settings_parser.cc',
        'browser/chromeos/proxy_cros_settings_parser.h',
        'browser/chromeos/settings/cros_settings.cc',
        'browser/chromeos/settings/cros_settings.h',
        'browser/chromeos/settings/cros_settings_names.cc',
        'browser/chromeos/settings/cros_settings_names.h',
        'browser/chromeos/settings/cros_settings_provider.cc',
        'browser/chromeos/settings/cros_settings_provider.h',
        'browser/chromeos/settings/device_settings_cache.cc',
        'browser/chromeos/settings/device_settings_cache.h',
        'browser/chromeos/settings/device_settings_provider.cc',
        'browser/chromeos/settings/device_settings_provider.h',
        'browser/chromeos/settings/device_settings_service.cc',
        'browser/chromeos/settings/device_settings_service.h',
        'browser/chromeos/settings/owner_key_util.cc',
        'browser/chromeos/settings/owner_key_util.h',
        'browser/chromeos/settings/session_manager_operation.cc',
        'browser/chromeos/settings/session_manager_operation.h',
        'browser/chromeos/settings/stub_cros_settings_provider.cc',
        'browser/chromeos/settings/stub_cros_settings_provider.h',
        'browser/chromeos/settings/system_settings_provider.cc',
        'browser/chromeos/settings/system_settings_provider.h',
        'browser/chromeos/sim_dialog_delegate.cc',
        'browser/chromeos/sim_dialog_delegate.h',
        'browser/chromeos/sms_observer.cc',
        'browser/chromeos/sms_observer.h',
        'browser/chromeos/status/data_promo_notification.cc',
        'browser/chromeos/status/data_promo_notification.h',
        'browser/chromeos/status/network_menu.cc',
        'browser/chromeos/status/network_menu.h',
        'browser/chromeos/status/network_menu_icon.cc',
        'browser/chromeos/status/network_menu_icon.h',
        'browser/chromeos/system/ash_system_tray_delegate.cc',
        'browser/chromeos/system/ash_system_tray_delegate.h',
        'browser/chromeos/system/drm_settings.cc',
        'browser/chromeos/system/drm_settings.h',
        'browser/chromeos/system/input_device_settings.cc',
        'browser/chromeos/system/input_device_settings.h',
        'browser/chromeos/system/name_value_pairs_parser.cc',
        'browser/chromeos/system/name_value_pairs_parser.h',
        'browser/chromeos/system/pointer_device_observer.cc',
        'browser/chromeos/system/pointer_device_observer.h',
        'browser/chromeos/system/power_manager_settings.cc',
        'browser/chromeos/system/power_manager_settings.h',
        'browser/chromeos/system/statistics_provider.cc',
        'browser/chromeos/system/statistics_provider.h',
        'browser/chromeos/system/syslogs_provider.cc',
        'browser/chromeos/system/syslogs_provider.h',
        'browser/chromeos/system/timezone_settings.cc',
        'browser/chromeos/system/timezone_settings.h',
        'browser/chromeos/system/udev_info_provider.cc',
        'browser/chromeos/system/udev_info_provider.h',
        'browser/chromeos/system_key_event_listener.cc',
        'browser/chromeos/system_key_event_listener.h',
        'browser/chromeos/system_logs/command_line_log_source.cc',
        'browser/chromeos/system_logs/command_line_log_source.h',
        'browser/chromeos/system_logs/debug_daemon_log_source.cc',
        'browser/chromeos/system_logs/debug_daemon_log_source.h',
        'browser/chromeos/system_logs/lsb_release_log_source.cc',
        'browser/chromeos/system_logs/lsb_release_log_source.h',
        'browser/chromeos/system_logs/memory_details_log_source.cc',
        'browser/chromeos/system_logs/memory_details_log_source.h',
        'browser/chromeos/system_logs/system_logs_fetcher.cc',
        'browser/chromeos/system_logs/system_logs_fetcher.h',
        'browser/chromeos/ui/idle_logout_dialog_view.cc',
        'browser/chromeos/ui/idle_logout_dialog_view.h',
        'browser/chromeos/upgrade_detector_chromeos.cc',
        'browser/chromeos/upgrade_detector_chromeos.h',
        'browser/chromeos/version_loader.cc',
        'browser/chromeos/version_loader.h',
        'browser/chromeos/view_ids.h',
        'browser/chromeos/web_socket_proxy.cc',
        'browser/chromeos/web_socket_proxy.h',
        'browser/chromeos/web_socket_proxy_controller.cc',
        'browser/chromeos/web_socket_proxy_controller.h',
        'browser/chromeos/web_socket_proxy_helper.cc',
        'browser/chromeos/web_socket_proxy_helper.h',
        'browser/chromeos/xinput_hierarchy_changed_event_listener.cc',
        'browser/chromeos/xinput_hierarchy_changed_event_listener.h',
        'browser/chromeos/xinput_hierarchy_changed_event_listener_aura.cc',
      ],
      'conditions': [
        ['enable_extensions==1', {
          'sources': [
            # Only extension API implementations should go here.
            'browser/chromeos/extensions/echo_private_api.cc',
            'browser/chromeos/extensions/echo_private_api.h',
            'browser/chromeos/extensions/file_browser_handler_api.cc',
            'browser/chromeos/extensions/file_browser_handler_api.h',
            'browser/chromeos/extensions/file_browser_private_api.cc',
            'browser/chromeos/extensions/file_browser_private_api.h',
            'browser/chromeos/extensions/wallpaper_manager_util.cc',
            'browser/chromeos/extensions/wallpaper_manager_util.h',
            'browser/chromeos/extensions/wallpaper_private_api.cc',
            'browser/chromeos/extensions/wallpaper_private_api.h',
            'browser/chromeos/media/media_player_extension_api.cc',
            'browser/chromeos/media/media_player_extension_api.h',
          ],
        }],
        ['use_cras==1', {
          'cflags': [
            '<!@(<(pkg-config) --cflags libcras)',
          ],
          'link_settings': {
            'libraries': [
              '<!@(<(pkg-config) --libs libcras)',
            ],
          },
          'defines': [
            'USE_CRAS',
          ],
          'sources/': [
            ['exclude', '^browser/chromeos/audio/audio_mixer_alsa.cc'],
            ['exclude', '^browser/chromeos/audio/audio_mixer_alsa.h'],
          ],
        }, {  # use_cras==0
          'sources/': [
            ['exclude', '^browser/chromeos/audio/audio_mixer_cras.cc'],
            ['exclude', '^browser/chromeos/audio/audio_mixer_cras.h'],
          ],
        }],
        ['file_manager_extension==0', {
          'sources/': [
            ['exclude', 'browser/chromeos/media/media_player_extension_api.cc'],
            ['exclude', 'browser/chromeos/media/media_player_extension_api.h'],
            ['exclude', 'browser/chromeos/extensions/file_browser_handler_api.cc'],
            ['exclude', 'browser/chromeos/extensions/file_browser_handler_api.h'],
            ['exclude', 'browser/chromeos/extensions/file_browser_private_api.cc'],
            ['exclude', 'browser/chromeos/extensions/file_browser_private_api.h'],
            ['exclude', 'browser/chromeos/extensions/file_handler_util.h'],
            ['exclude', 'browser/chromeos/extensions/file_handler_util.cc'],
            ['exclude', 'browser/chromeos/extensions/file_manager_util.h'],
            ['exclude', 'browser/chromeos/extensions/file_manager_util.cc'],
          ],
        }],
        ['use_ibus==1', {
          'dependencies': [
            '../build/linux/system.gyp:ibus',
          ],
        }],
        ['use_ash==1', {
          'dependencies': [
            '../ash/ash.gyp:ash',
            '../ash/ash_strings.gyp:ash_strings',
            '../ui/app_list/app_list.gyp:app_list',
          ],
        }],
        ['use_aura==1', {
          'dependencies': [
            '../ui/aura/aura.gyp:aura',
            '../ui/compositor/compositor.gyp:compositor',
          ],
        }],
        ['ui_compositor_image_transport==1', {
          'dependencies': [
            '../ui/gl/gl.gyp:gl',
          ],
        }],
        ['linux_breakpad==1', {
          'dependencies': [
            '../breakpad/breakpad.gyp:breakpad_client',
            # make sure file_version_info_linux.h is generated first.
            'common',
          ],
          'include_dirs': [
            # breakpad_linux.cc uses generated file_version_info_linux.h.
            '<(SHARED_INTERMEDIATE_DIR)',
            '../breakpad/src',
          ],
        }],
        ['use_aura==1',{
          'dependencies': [
            '../build/linux/system.gyp:dbus',
            '../build/linux/system.gyp:fontconfig',
            '../build/linux/system.gyp:x11',
            '../ui/views/views.gyp:views',
          ],
          'include_dirs': [
            '<(INTERMEDIATE_DIR)',
            '<(INTERMEDIATE_DIR)/chrome',
          ],
          'sources/': [
            ['include', '^browser/chromeos/status/memory_menu_button.cc'],
            ['include', '^browser/chromeos/status/memory_menu_button.h'],
          ],
        }],
      ],
    },
    {
      # Protobuf compiler / generator for the Drive protocol buffer.
      'target_name': 'drive_proto',
      'type': 'static_library',
      'sources': [ 'browser/chromeos/drive/drive.proto' ],
      'variables': {
        'proto_in_dir': 'browser/chromeos/drive',
        'proto_out_dir': 'chrome/browser/chromeos/drive',
      },
      'includes': [ '../build/protoc.gypi' ]
    },
  ],
}
