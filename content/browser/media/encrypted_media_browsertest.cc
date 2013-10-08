// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "content/browser/media/media_browsertest.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"

// Available key systems.
static const char kClearKeyKeySystem[] = "webkit-org.w3.clearkey";

// Supported media types.
static const char kWebMAudioOnly[] = "audio/webm; codecs=\"vorbis\"";
static const char kWebMVideoOnly[] = "video/webm; codecs=\"vp8\"";
static const char kWebMAudioVideo[] = "video/webm; codecs=\"vorbis, vp8\"";

// EME-specific test results and errors.
static const char kEmeGkrException[] = "GENERATE_KEY_REQUEST_EXCEPTION";
static const char kEmeKeyError[] = "KEYERROR";

// The type of video src used to load media.
enum SrcType {
  SRC,
  MSE
};

namespace content {

// Tests encrypted media playback with a combination of parameters:
// - char*: Key system name.
// - bool: True to load media using MSE, otherwise use src.
class EncryptedMediaTest : public content::MediaBrowserTest,
    public testing::WithParamInterface<std::tr1::tuple<const char*, SrcType> > {
 public:
  // Can only be used in parameterized (*_P) tests.
  const char* CurrentKeySystem() {
    return std::tr1::get<0>(GetParam());
  }

  // Can only be used in parameterized (*_P) tests.
  SrcType CurrentSourceType() {
    return std::tr1::get<1>(GetParam());
  }

  void TestSimplePlayback(const char* encrypted_media, const char* media_type) {
    RunSimpleEncryptedMediaTest(
        encrypted_media, media_type, CurrentKeySystem(), CurrentSourceType());
  }

  void TestFrameSizeChange() {
    RunEncryptedMediaTest("encrypted_frame_size_change.html",
                          "frame_size_change-av-enc-v.webm", kWebMAudioVideo,
                          CurrentKeySystem(), CurrentSourceType(), kEnded);
  }

  void TestConfigChange() {
    std::vector<StringPair> query_params;
    query_params.push_back(std::make_pair("keysystem", CurrentKeySystem()));
    query_params.push_back(std::make_pair("runencrypted", "1"));
    RunMediaTestPage("mse_config_change.html", &query_params, kEnded, true);
  }

  void RunEncryptedMediaTest(const char* html_page,
                             const char* media_file,
                             const char* media_type,
                             const char* key_system,
                             SrcType src_type,
                             const char* expectation) {
    std::vector<StringPair> query_params;
    query_params.push_back(std::make_pair("mediafile", media_file));
    query_params.push_back(std::make_pair("mediatype", media_type));
    query_params.push_back(std::make_pair("keysystem", key_system));
    if (src_type == MSE)
      query_params.push_back(std::make_pair("usemse", "1"));
    RunMediaTestPage(html_page, &query_params, expectation, true);
  }

  void RunSimpleEncryptedMediaTest(const char* media_file,
                                   const char* media_type,
                                   const char* key_system,
                                   SrcType src_type) {
    RunEncryptedMediaTest("encrypted_media_player.html", media_file,
                          media_type, key_system, src_type, kEnded);
  }

 protected:
  // We want to fail quickly when a test fails because an error is encountered.
  virtual void AddWaitForTitles(content::TitleWatcher* title_watcher) OVERRIDE {
    MediaBrowserTest::AddWaitForTitles(title_watcher);
    title_watcher->AlsoWaitForTitle(ASCIIToUTF16(kEmeGkrException));
    title_watcher->AlsoWaitForTitle(ASCIIToUTF16(kEmeKeyError));
  }

#if defined(OS_ANDROID)
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitch(
        switches::kDisableGestureRequirementForMediaPlayback);
  }
#endif
};

INSTANTIATE_TEST_CASE_P(ClearKey, EncryptedMediaTest,
    ::testing::Combine(
        ::testing::Values(kClearKeyKeySystem), ::testing::Values(SRC, MSE)));

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioOnly_WebM) {
  TestSimplePlayback("bear-a-enc_a.webm", kWebMAudioOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioClearVideo_WebM) {
  TestSimplePlayback("bear-320x240-av-enc_a.webm", kWebMAudioVideo);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoAudio_WebM) {
  TestSimplePlayback("bear-320x240-av-enc_av.webm", kWebMAudioVideo);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoOnly_WebM) {
  TestSimplePlayback("bear-320x240-v-enc_v.webm", kWebMVideoOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoClearAudio_WebM) {
  TestSimplePlayback("bear-320x240-av-enc_v.webm", kWebMAudioVideo);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, ConfigChangeVideo) {
  TestConfigChange();
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, FrameSizeChangeVideo) {
  // Times out on Windows XP. http://crbug.com/171937
#if defined(OS_WIN)
  if (base::win::GetVersion() < base::win::VERSION_VISTA)
    return;
#endif
  TestFrameSizeChange();
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaTest, UnknownKeySystemThrowsException) {
  RunEncryptedMediaTest("encrypted_media_player.html", "bear-a-enc_a.webm",
                        kWebMAudioOnly, "com.example.foo", SRC,
                        kEmeGkrException);
}

}  // namespace content
