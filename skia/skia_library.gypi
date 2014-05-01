# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# This gypi file contains the Skia library.
# In component mode (shared_lib) it is folded into a single shared library with
# the Chrome-specific enhancements but in all other cases it is a separate lib.

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# variables and defines should go in skia_common.gypi so they can be seen
# by files listed here and in skia_library_opts.gypi.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
  'dependencies': [
    'skia_library_opts.gyp:skia_opts',
    '../third_party/zlib/zlib.gyp:zlib',
  ],

  'includes': [
    '../third_party/skia/gyp/core.gypi',
    '../third_party/skia/gyp/effects.gypi',
    '../third_party/skia/gyp/pdf.gypi',
    '../third_party/skia/gyp/record.gypi',
  ],

  'sources': [
    # this should likely be moved into src/utils in skia
    '../third_party/skia/src/core/SkFlate.cpp',
    '../third_party/skia/src/core/SkPaintOptionsAndroid.cpp',

    '../third_party/skia/src/ports/SkImageDecoder_empty.cpp',
    '../third_party/skia/src/images/SkScaledBitmapSampler.cpp',
    '../third_party/skia/src/images/SkScaledBitmapSampler.h',

    '../third_party/skia/src/opts/opts_check_x86.cpp',

    '../third_party/skia/src/ports/SkFontConfigInterface_android.cpp',
    '../third_party/skia/src/ports/SkFontConfigInterface_direct.cpp',

    '../third_party/skia/src/fonts/SkFontMgr_fontconfig.cpp',
    '../third_party/skia/src/ports/SkFontHost_fontconfig.cpp',

    '../third_party/skia/src/fonts/SkFontMgr_indirect.cpp',
    '../third_party/skia/src/fonts/SkRemotableFontMgr.cpp',
    '../third_party/skia/src/ports/SkRemotableFontMgr_win_dw.cpp',

    '../third_party/skia/src/ports/SkFontHost_FreeType.cpp',
    '../third_party/skia/src/ports/SkFontHost_FreeType_common.cpp',
    '../third_party/skia/src/ports/SkFontHost_FreeType_common.h',
    '../third_party/skia/src/ports/SkFontConfigParser_android.cpp',
    '../third_party/skia/src/ports/SkFontHost_mac.cpp',
    '../third_party/skia/src/ports/SkFontHost_win.cpp',
    '../third_party/skia/src/ports/SkFontHost_win_dw.cpp',
    '../third_party/skia/src/ports/SkFontMgr_default_gdi.cpp',
    '../third_party/skia/src/ports/SkGlobalInitialization_chromium.cpp',
    '../third_party/skia/src/ports/SkOSFile_posix.cpp',
    '../third_party/skia/src/ports/SkOSFile_stdio.cpp',
    '../third_party/skia/src/ports/SkOSFile_win.cpp',
    '../third_party/skia/src/ports/SkTime_Unix.cpp',
    '../third_party/skia/src/ports/SkTLS_pthread.cpp',
    '../third_party/skia/src/ports/SkTLS_win.cpp',

    '../third_party/skia/src/sfnt/SkOTTable_name.cpp',
    '../third_party/skia/src/sfnt/SkOTTable_name.h',
    '../third_party/skia/src/sfnt/SkOTUtils.cpp',
    '../third_party/skia/src/sfnt/SkOTUtils.h',

    '../third_party/skia/include/utils/mac/SkCGUtils.h',
    '../third_party/skia/include/utils/SkDeferredCanvas.h',
    '../third_party/skia/include/utils/SkMatrix44.h',
    '../third_party/skia/include/utils/SkNoSaveLayerCanvas.h',
    '../third_party/skia/src/utils/debugger/SkDebugCanvas.cpp',
    '../third_party/skia/src/utils/debugger/SkDebugCanvas.h',
    '../third_party/skia/src/utils/debugger/SkDrawCommand.cpp',
    '../third_party/skia/src/utils/debugger/SkDrawCommand.h',
    '../third_party/skia/src/utils/debugger/SkObjectParser.cpp',
    '../third_party/skia/src/utils/debugger/SkObjectParser.h',
    '../third_party/skia/src/utils/mac/SkCreateCGImageRef.cpp',
    '../third_party/skia/src/utils/SkBase64.cpp',
    '../third_party/skia/src/utils/SkBase64.h',
    '../third_party/skia/src/utils/SkBitSet.cpp',
    '../third_party/skia/src/utils/SkBitSet.h',
    '../third_party/skia/src/utils/SkCanvasStack.cpp',
    '../third_party/skia/src/utils/SkCanvasStateUtils.cpp',
    '../third_party/skia/src/utils/SkEventTracer.cpp',
    '../third_party/skia/src/utils/SkDeferredCanvas.cpp',
    '../third_party/skia/src/utils/SkMatrix22.cpp',
    '../third_party/skia/src/utils/SkMatrix22.h',
    '../third_party/skia/src/utils/SkMatrix44.cpp',
    '../third_party/skia/src/utils/SkNullCanvas.cpp',
    '../third_party/skia/include/utils/SkNWayCanvas.h',
    '../third_party/skia/src/utils/SkNWayCanvas.cpp',
    '../third_party/skia/src/utils/SkPictureUtils.cpp',
    '../third_party/skia/src/utils/SkProxyCanvas.cpp',
    '../third_party/skia/src/utils/SkRTConf.cpp',
    '../third_party/skia/include/utils/SkRTConf.h',
    '../third_party/skia/src/utils/win/SkDWrite.h',
    '../third_party/skia/src/utils/win/SkDWrite.cpp',
    '../third_party/skia/src/utils/win/SkDWriteFontFileStream.cpp',
    '../third_party/skia/src/utils/win/SkDWriteFontFileStream.h',
    '../third_party/skia/src/utils/win/SkDWriteGeometrySink.cpp',
    '../third_party/skia/src/utils/win/SkDWriteGeometrySink.h',
    '../third_party/skia/src/utils/win/SkHRESULT.cpp',

    '../third_party/skia/include/images/SkImageRef.h',
    '../third_party/skia/include/images/SkImageRef_GlobalPool.h',
    '../third_party/skia/include/images/SkMovie.h',
    '../third_party/skia/include/images/SkPageFlipper.h',

    '../third_party/skia/include/ports/SkFontConfigInterface.h',
    '../third_party/skia/include/ports/SkFontMgr.h',
    '../third_party/skia/include/ports/SkFontMgr_indirect.h',
    '../third_party/skia/include/ports/SkFontStyle.h',
    '../third_party/skia/include/ports/SkRemotableFontMgr.h',
    '../third_party/skia/include/ports/SkTypeface_win.h',

    '../third_party/skia/include/utils/SkNullCanvas.h',
    '../third_party/skia/include/utils/SkPictureUtils.h',
    '../third_party/skia/include/utils/SkProxyCanvas.h',
  ],
  'include_dirs': [
    '../third_party/skia/include/core',
    '../third_party/skia/include/effects',
    '../third_party/skia/include/images',
    '../third_party/skia/include/lazy',
    '../third_party/skia/include/pathops',
    '../third_party/skia/include/pdf',
    '../third_party/skia/include/pipe',
    '../third_party/skia/include/ports',
    '../third_party/skia/include/record',
    '../third_party/skia/include/utils',
    '../third_party/skia/src/core',
    '../third_party/skia/src/opts',
    '../third_party/skia/src/image',
    '../third_party/skia/src/ports',
    '../third_party/skia/src/sfnt',
    '../third_party/skia/src/utils',
    '../third_party/skia/src/lazy',
  ],
  'conditions': [
    ['skia_support_gpu != 0', {
      'includes': [
        '../third_party/skia/gyp/gpu.gypi',
      ],
      'sources': [
        '<@(skgpu_null_gl_sources)',
        '<@(skgpu_sources)',
      ],
      'include_dirs': [
        '../third_party/skia/include/gpu',
        '../third_party/skia/src/gpu',
      ],
    }],
    ['skia_support_pdf == 0', {
      'sources/': [
        ['exclude', '../third_party/skia/src/pdf/']
      ],
    }],
    ['skia_support_pdf == 1', {
      'dependencies': [
        '../third_party/sfntly/sfntly.gyp:sfntly',
      ],
    }],

    [ 'OS != "ios"', {
      'dependencies': [
        '../third_party/WebKit/public/blink_skia_config.gyp:blink_skia_config',
      ],
      'export_dependent_settings': [
        '../third_party/WebKit/public/blink_skia_config.gyp:blink_skia_config',
      ],
    }],
    [ 'OS != "mac"', {
      'sources/': [
        ['exclude', '/mac/']
      ],
    }],
    [ 'OS == "android" and target_arch == "arm"', {
      'sources': [
        '../third_party/skia/src/core/SkUtilsArm.cpp',
      ],
      'includes': [
        '../build/android/cpufeatures.gypi',
      ],
    }],
    [ 'target_arch == "arm" or target_arch == "arm64" or \
       target_arch == "mipsel"', {
      'sources!': [
        '../third_party/skia/src/opts/opts_check_x86.cpp'
      ],
    }],
    [ 'desktop_linux == 1 or chromeos == 1', {
      'dependencies': [
        '../build/linux/system.gyp:fontconfig',
        '../build/linux/system.gyp:freetype2',
        '../third_party/icu/icu.gyp:icuuc',
      ],
      'cflags': [
        '-Wno-unused',
        '-Wno-unused-function',
      ],
    }],
    [ 'use_cairo == 1', {
      'dependencies': [
        '../build/linux/system.gyp:pangocairo',
      ],
    }],
    [ 'OS=="win" or OS=="mac" or OS=="ios" or OS=="android"', {
      'sources!': [
        '../third_party/skia/src/ports/SkFontConfigInterface_direct.cpp',
        '../third_party/skia/src/fonts/SkFontMgr_fontconfig.cpp',
      ],
    }],
    [ 'OS=="win" or OS=="mac" or OS=="ios"', {
      'sources!': [
        '../third_party/skia/src/ports/SkFontHost_FreeType.cpp',
        '../third_party/skia/src/ports/SkFontHost_FreeType_common.cpp',
        '../third_party/skia/src/ports/SkFontHost_fontconfig.cpp',

      ],
    }],
    [ 'OS == "android"', {
      'dependencies': [
        '../third_party/expat/expat.gyp:expat',
        '../third_party/freetype/freetype.gyp:ft2',
      ],
      # This exports a hard dependency because it needs to run its
      # symlink action in order to expose the skia header files.
      'hard_dependency': 1,
      'include_dirs': [
        '../third_party/expat/files/lib',
      ],
    }],
    [ 'OS == "ios"', {
      'include_dirs': [
        '../third_party/skia/include/utils/ios',
        '../third_party/skia/include/utils/mac',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/ImageIO.framework',
        ],
      },
      'sources': [
        # This file is used on both iOS and Mac, so it should be removed
        #  from the ios and mac conditions and moved into the main sources
        #  list.
        '../third_party/skia/src/utils/mac/SkStream_mac.cpp',
      ],
      'sources/': [
        ['exclude', 'opts_check_x86\\.cpp$'],
      ],

      # The main skia_opts target does not currently work on iOS because the
      # target architecture on iOS is determined at compile time rather than
      # gyp time (simulator builds are x86, device builds are arm).  As a
      # temporary measure, this is a separate opts target for iOS-only, using
      # the _none.cpp files to avoid architecture-dependent implementations.
      'dependencies': [
        'skia_library_opts.gyp:skia_opts_none',
      ],
      'dependencies!': [
        'skia_library_opts.gyp:skia_opts',
      ],
    }],
    [ 'OS == "mac"', {
      'direct_dependent_settings': {
        'include_dirs': [
          '../third_party/skia/include/utils/mac',
        ],
      },
      'include_dirs': [
        '../third_party/skia/include/utils/mac',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
        ],
      },
      'sources': [
        '../third_party/skia/src/utils/mac/SkStream_mac.cpp',
      ],
    }],
    [ 'OS == "win"', {
      'sources!': [
        '../third_party/skia/src/ports/SkOSFile_posix.cpp',
        '../third_party/skia/src/ports/SkTime_Unix.cpp',
        '../third_party/skia/src/ports/SkTLS_pthread.cpp',
      ],
      'include_dirs': [
        '../third_party/skia/include/utils/win',
        '../third_party/skia/src/utils/win',
      ],
    },{ # not 'OS == "win"'
      'sources!': [
        '../third_party/skia/src/ports/SkFontHost_win_dw.cpp',
        '../third_party/skia/src/ports/SkFontMgr_default_gdi.cpp',
        '../third_party/skia/src/ports/SkRemotableFontMgr_win_dw.cpp',

        '../third_party/skia/src/utils/win/SkDWrite.h',
        '../third_party/skia/src/utils/win/SkDWrite.cpp',
        '../third_party/skia/src/utils/win/SkDWriteFontFileStream.cpp',
        '../third_party/skia/src/utils/win/SkDWriteFontFileStream.h',
        '../third_party/skia/src/utils/win/SkDWriteGeometrySink.cpp',
        '../third_party/skia/src/utils/win/SkDWriteGeometrySink.h',
        '../third_party/skia/src/utils/win/SkHRESULT.cpp',
      ],
    }],
    # TODO(scottmg): http://crbug.com/177306
    ['clang==1', {
      'xcode_settings': {
        'WARNING_CFLAGS!': [
          # Don't warn about string->bool used in asserts.
          '-Wstring-conversion',
        ],
      },
      'cflags!': [
        '-Wstring-conversion',
      ],
    }],
  ],
  'target_conditions': [
    # Pull in specific Mac files for iOS (which have been filtered out
    # by file name rules).
    [ 'OS == "ios"', {
      'sources/': [
        ['include', 'SkFontHost_mac\\.cpp$',],
        ['include', 'SkStream_mac\\.cpp$',],
        ['include', 'SkCreateCGImageRef\\.cpp$',],
      ],
    }],
  ],

  'direct_dependent_settings': {
    'include_dirs': [
      #temporary until we can hide SkFontHost
      '../third_party/skia/src/core',

      '../third_party/skia/include/core',
      '../third_party/skia/include/effects',
      '../third_party/skia/include/pdf',
      '../third_party/skia/include/gpu',
      '../third_party/skia/include/lazy',
      '../third_party/skia/include/pathops',
      '../third_party/skia/include/pipe',
      '../third_party/skia/include/ports',
      '../third_party/skia/include/utils',
    ],
  },
}
