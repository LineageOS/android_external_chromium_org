# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This gyp file creates a fake target that is used to generate a minimal Xcode
# project, useful for editing XIB files.
#
# The sole target is called "chrome_nibs" and its sources are the minimum
# dependency set for all of the classes referred to by XIB files. If you are
# editing or adding a new XIB file, ensure that any classes to which you refer
# in the XIB are listed (both header and implementation) here so that Xcode can
# connect them.
#
# This target DOES NOT BUILD. Attempting to do so will generate lots of errors.
# Only use this target for editing XIBs.
#
# For more information, see
# <http://dev.chromium.org/developers/design-documents/mac-xib-files>.
{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common.gypi',
    'chrome_nibs.gypi',
  ],
  'target_defaults': {
    'include_dirs': [
      '..',
    ],
  },
  'targets': [
    {
      'target_name': 'chrome_nibs',
      'type': 'executable',
      'mac_bundle': 1,
      'sources': [
        '../third_party/GTM/AppKit/GTMUILocalizer.h',
        '../third_party/GTM/AppKit/GTMUILocalizer.mm',
        '../third_party/GTM/AppKit/GTMUILocalizerAndLayoutTweaker.h',
        '../third_party/GTM/AppKit/GTMUILocalizerAndLayoutTweaker.mm',
        '../ui/base/cocoa/base_view.h',
        '../ui/base/cocoa/base_view.mm',
        '../ui/base/cocoa/hover_button.h',
        '../ui/base/cocoa/hover_button.mm',
        '../ui/base/cocoa/hover_image_button.h',
        '../ui/base/cocoa/hover_image_button.mm',
        'browser/ui/cocoa/about_ipc_controller.h',
        'browser/ui/cocoa/about_ipc_controller.mm',
        'browser/ui/cocoa/animatable_view.h',
        'browser/ui/cocoa/animatable_view.mm',
        'browser/ui/cocoa/background_gradient_view.h',
        'browser/ui/cocoa/background_gradient_view.mm',
        'browser/ui/cocoa/base_bubble_controller.h',
        'browser/ui/cocoa/base_bubble_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_all_tabs_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_all_tabs_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_view.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_view.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_unittest_helper.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_unittest_helper.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bar_view.h',
        'browser/ui/cocoa/bookmarks/bookmark_bar_view.mm',
        'browser/ui/cocoa/bookmarks/bookmark_bubble_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_bubble_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_button.h',
        'browser/ui/cocoa/bookmarks/bookmark_button.mm',
        'browser/ui/cocoa/bookmarks/bookmark_button_cell.h',
        'browser/ui/cocoa/bookmarks/bookmark_button_cell.mm',
        'browser/ui/cocoa/bookmarks/bookmark_editor_base_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_editor_base_controller.mm',
        'browser/ui/cocoa/bookmarks/bookmark_name_folder_controller.h',
        'browser/ui/cocoa/bookmarks/bookmark_name_folder_controller.mm',
        'browser/ui/cocoa/browser/avatar_menu_bubble_controller.h',
        'browser/ui/cocoa/browser/avatar_menu_bubble_controller.mm',
        'browser/ui/cocoa/browser_window_controller.h',
        'browser/ui/cocoa/browser_window_controller.mm',
        'browser/ui/cocoa/browser_window_controller_private.h',
        'browser/ui/cocoa/browser_window_controller_private.mm',
        'browser/ui/cocoa/chrome_browser_window.h',
        'browser/ui/cocoa/chrome_browser_window.mm',
        'browser/ui/cocoa/chrome_event_processing_window.h',
        'browser/ui/cocoa/chrome_event_processing_window.mm',
        'browser/ui/cocoa/chrome_to_mobile_bubble_controller.h',
        'browser/ui/cocoa/chrome_to_mobile_bubble_controller.mm',
        'browser/ui/cocoa/clickhold_button_cell.h',
        'browser/ui/cocoa/clickhold_button_cell.mm',
        'browser/ui/cocoa/content_settings/collected_cookies_mac.h',
        'browser/ui/cocoa/content_settings/collected_cookies_mac.mm',
        'browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.h',
        'browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.mm',
        'browser/ui/cocoa/content_settings/cookie_details_view_controller.h',
        'browser/ui/cocoa/content_settings/cookie_details_view_controller.mm',
        'browser/ui/cocoa/custom_frame_view.h',
        'browser/ui/cocoa/custom_frame_view.mm',
        'browser/ui/cocoa/download/download_item_button.h',
        'browser/ui/cocoa/download/download_item_button.mm',
        'browser/ui/cocoa/download/download_item_cell.h',
        'browser/ui/cocoa/download/download_item_cell.mm',
        'browser/ui/cocoa/download/download_item_controller.h',
        'browser/ui/cocoa/download/download_item_controller.mm',
        'browser/ui/cocoa/download/download_shelf_controller.h',
        'browser/ui/cocoa/download/download_shelf_controller.mm',
        'browser/ui/cocoa/download/download_shelf_view.h',
        'browser/ui/cocoa/download/download_shelf_view.mm',
        'browser/ui/cocoa/download/download_show_all_button.h',
        'browser/ui/cocoa/download/download_show_all_button.mm',
        'browser/ui/cocoa/download/download_show_all_cell.h',
        'browser/ui/cocoa/download/download_show_all_cell.mm',
        'browser/ui/cocoa/draggable_button.h',
        'browser/ui/cocoa/draggable_button.mm',
        'browser/ui/cocoa/browser/edit_search_engine_cocoa_controller.h',
        'browser/ui/cocoa/browser/edit_search_engine_cocoa_controller.mm',
        'browser/ui/cocoa/constrained_window/constrained_window_button.h',
        'browser/ui/cocoa/constrained_window/constrained_window_button.mm',
        'browser/ui/cocoa/constrained_window/constrained_window_custom_window.h',
        'browser/ui/cocoa/constrained_window/constrained_window_custom_window.mm',
        'browser/ui/cocoa/extensions/browser_actions_container_view.h',
        'browser/ui/cocoa/extensions/browser_actions_container_view.mm',
        'browser/ui/cocoa/extensions/extension_install_dialog_controller.h',
        'browser/ui/cocoa/extensions/extension_install_dialog_controller.mm',
        'browser/ui/cocoa/extensions/extension_install_view_controller.h',
        'browser/ui/cocoa/extensions/extension_install_view_controller.mm',
        'browser/ui/cocoa/extensions/extension_installed_bubble_controller.h',
        'browser/ui/cocoa/extensions/extension_installed_bubble_controller.mm',
        'browser/ui/cocoa/fast_resize_view.h',
        'browser/ui/cocoa/fast_resize_view.mm',
        'browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h',
        'browser/ui/cocoa/find_bar/find_bar_cocoa_controller.mm',
        'browser/ui/cocoa/find_bar/find_bar_text_field.h',
        'browser/ui/cocoa/find_bar/find_bar_text_field.mm',
        'browser/ui/cocoa/find_bar/find_bar_text_field_cell.h',
        'browser/ui/cocoa/find_bar/find_bar_text_field_cell.mm',
        'browser/ui/cocoa/find_bar/find_bar_view.h',
        'browser/ui/cocoa/find_bar/find_bar_view.mm',
        'browser/ui/cocoa/first_run_bubble_controller.h',
        'browser/ui/cocoa/first_run_bubble_controller.mm',
        'browser/ui/cocoa/first_run_dialog.h',
        'browser/ui/cocoa/first_run_dialog.mm',
        'browser/ui/cocoa/framed_browser_window.h',
        'browser/ui/cocoa/framed_browser_window.mm',
        'browser/ui/cocoa/fullscreen_exit_bubble_controller.h',
        'browser/ui/cocoa/fullscreen_exit_bubble_controller.mm',
        'browser/ui/cocoa/fullscreen_exit_bubble_view.h',
        'browser/ui/cocoa/fullscreen_exit_bubble_view.mm',
        'browser/ui/cocoa/global_error_bubble_controller.h',
        'browser/ui/cocoa/global_error_bubble_controller.mm',
        'browser/ui/cocoa/gradient_button_cell.h',
        'browser/ui/cocoa/gradient_button_cell.mm',
        'browser/ui/cocoa/hover_close_button.h',
        'browser/ui/cocoa/hover_close_button.mm',
        'browser/ui/cocoa/hung_renderer_controller.h',
        'browser/ui/cocoa/hung_renderer_controller.mm',
        'browser/ui/cocoa/hyperlink_button_cell.h',
        'browser/ui/cocoa/hyperlink_button_cell.mm',
        'browser/ui/cocoa/image_button_cell.h',
        'browser/ui/cocoa/image_button_cell.mm',
        'browser/ui/cocoa/importer/import_progress_dialog_cocoa.h',
        'browser/ui/cocoa/importer/import_progress_dialog_cocoa.mm',
        'browser/ui/cocoa/info_bubble_view.h',
        'browser/ui/cocoa/info_bubble_view.mm',
        'browser/ui/cocoa/info_bubble_window.h',
        'browser/ui/cocoa/info_bubble_window.mm',
        'browser/ui/cocoa/infobars/after_translate_infobar_controller.h',
        'browser/ui/cocoa/infobars/after_translate_infobar_controller.mm',
        'browser/ui/cocoa/infobars/alternate_nav_infobar_controller.h',
        'browser/ui/cocoa/infobars/alternate_nav_infobar_controller.mm',
        'browser/ui/cocoa/infobars/before_translate_infobar_controller.h',
        'browser/ui/cocoa/infobars/before_translate_infobar_controller.mm',
        'browser/ui/cocoa/infobars/confirm_infobar_controller.h',
        'browser/ui/cocoa/infobars/confirm_infobar_controller.mm',
        'browser/ui/cocoa/infobars/extension_infobar_controller.h',
        'browser/ui/cocoa/infobars/extension_infobar_controller.mm',
        'browser/ui/cocoa/infobars/infobar_container_controller.h',
        'browser/ui/cocoa/infobars/infobar_container_controller.mm',
        'browser/ui/cocoa/infobars/infobar_controller.h',
        'browser/ui/cocoa/infobars/infobar_controller.mm',
        'browser/ui/cocoa/infobars/infobar_gradient_view.h',
        'browser/ui/cocoa/infobars/infobar_gradient_view.mm',
        'browser/ui/cocoa/location_bar/action_box_menu_bubble_controller.h',
        'browser/ui/cocoa/location_bar/action_box_menu_bubble_controller.mm',
        'browser/ui/cocoa/location_bar/autocomplete_text_field.h',
        'browser/ui/cocoa/location_bar/autocomplete_text_field.mm',
        'browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h',
        'browser/ui/cocoa/location_bar/autocomplete_text_field_cell.mm',
        'browser/ui/cocoa/login_prompt_cocoa.h',
        'browser/ui/cocoa/login_prompt_cocoa.mm',
        'browser/ui/cocoa/menu_button.h',
        'browser/ui/cocoa/menu_button.mm',
        'browser/ui/cocoa/menu_controller.h',
        'browser/ui/cocoa/menu_controller.mm',
        'browser/ui/cocoa/multi_key_equivalent_button.h',
        'browser/ui/cocoa/multi_key_equivalent_button.mm',
        'browser/ui/cocoa/new_tab_button.h',
        'browser/ui/cocoa/new_tab_button.mm',
        'browser/ui/cocoa/notifications/balloon_controller.h',
        'browser/ui/cocoa/notifications/balloon_controller.mm',
        'browser/ui/cocoa/notifications/balloon_view.h',
        'browser/ui/cocoa/notifications/balloon_view.mm',
        'browser/ui/cocoa/nsmenuitem_additions.h',
        'browser/ui/cocoa/nsmenuitem_additions.mm',
        'browser/ui/cocoa/nsview_additions.h',
        'browser/ui/cocoa/nsview_additions.mm',
        'browser/ui/cocoa/one_click_signin_view_controller.h',
        'browser/ui/cocoa/one_click_signin_view_controller.mm',
        'browser/ui/cocoa/screen_capture_notification_ui_cocoa.h',
        'browser/ui/cocoa/screen_capture_notification_ui_cocoa.mm',
        'browser/ui/cocoa/speech_recognition_window_controller.h',
        'browser/ui/cocoa/speech_recognition_window_controller.mm',
        'browser/ui/cocoa/status_bubble_mac.h',
        'browser/ui/cocoa/status_bubble_mac.mm',
        'browser/ui/cocoa/styled_text_field.h',
        'browser/ui/cocoa/styled_text_field.mm',
        'browser/ui/cocoa/styled_text_field_cell.h',
        'browser/ui/cocoa/styled_text_field_cell.mm',
        'browser/ui/cocoa/tab_contents/overlayable_contents_controller.h',
        'browser/ui/cocoa/tab_contents/overlayable_contents_controller.mm',
        'browser/ui/cocoa/tab_contents/sad_tab_controller.h',
        'browser/ui/cocoa/tab_contents/sad_tab_controller.mm',
        'browser/ui/cocoa/tab_contents/sad_tab_view.h',
        'browser/ui/cocoa/tab_contents/sad_tab_view.mm',
        'browser/ui/cocoa/tabs/tab_controller.h',
        'browser/ui/cocoa/tabs/tab_controller.mm',
        'browser/ui/cocoa/tabs/tab_strip_model_observer_bridge.h',
        'browser/ui/cocoa/tabs/tab_strip_model_observer_bridge.mm',
        'browser/ui/cocoa/tabs/tab_strip_view.h',
        'browser/ui/cocoa/tabs/tab_strip_view.mm',
        'browser/ui/cocoa/tabs/tab_view.h',
        'browser/ui/cocoa/tabs/tab_view.mm',
        'browser/ui/cocoa/tabs/tab_window_controller.h',
        'browser/ui/cocoa/tabs/tab_window_controller.mm',
        'browser/ui/cocoa/task_manager_mac.h',
        'browser/ui/cocoa/task_manager_mac.mm',
        'browser/ui/cocoa/themed_window.h',
        'browser/ui/cocoa/themed_window.mm',
        'browser/ui/cocoa/toolbar/reload_button.h',
        'browser/ui/cocoa/toolbar/reload_button.mm',
        'browser/ui/cocoa/toolbar/toolbar_button.h',
        'browser/ui/cocoa/toolbar/toolbar_button.mm',
        'browser/ui/cocoa/toolbar/toolbar_controller.h',
        'browser/ui/cocoa/toolbar/toolbar_controller.mm',
        'browser/ui/cocoa/toolbar/toolbar_view.h',
        'browser/ui/cocoa/toolbar/toolbar_view.mm',
        'browser/ui/cocoa/toolbar/wrench_toolbar_button_cell.h',
        'browser/ui/cocoa/toolbar/wrench_toolbar_button_cell.mm',
        'browser/ui/cocoa/ui_localizer.h',
        'browser/ui/cocoa/ui_localizer.mm',
        'browser/ui/cocoa/vertical_gradient_view.h',
        'browser/ui/cocoa/vertical_gradient_view.mm',
        'browser/ui/cocoa/view_id_util.h',
        'browser/ui/cocoa/view_id_util.mm',
        'browser/ui/cocoa/wrench_menu/menu_tracked_root_view.h',
        'browser/ui/cocoa/wrench_menu/menu_tracked_root_view.mm',
        'browser/ui/cocoa/wrench_menu/wrench_menu_controller.h',
        'browser/ui/cocoa/wrench_menu/wrench_menu_controller.mm',
        'browser/ui/cocoa/panels/panel_titlebar_view_cocoa.h',
        'browser/ui/cocoa/panels/panel_titlebar_view_cocoa.mm',
        'browser/ui/cocoa/panels/panel_window_controller_cocoa.h',
        'browser/ui/cocoa/panels/panel_window_controller_cocoa.mm',
      ],
      'mac_bundle_resources': [
        '<@(mac_all_xibs)',
      ],
    },  # target chrome_xibs
  ],  # targets
}
