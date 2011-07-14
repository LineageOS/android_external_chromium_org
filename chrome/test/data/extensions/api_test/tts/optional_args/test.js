// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// browser_tests.exe --gtest_filter="TtsApiTest.*"

chrome.test.runTests([
  function testSpeakWithOptionalArgs() {
    // This will fail.
    try {
      chrome.experimental.tts.speak();
      chrome.test.fail();
    } catch (e) {
    }

    // This will succeed but nothing will be spoken.
    chrome.experimental.tts.speak('');

    // This will succeed.
    chrome.experimental.tts.speak('Alpha');

    // This will fail.
    try {
      chrome.experimental.tts.speak(null);
      chrome.test.fail();
    } catch (e) {
    }

    // This will succeed.
    chrome.experimental.tts.speak('Bravo', {});

    // This will succeed.
    chrome.experimental.tts.speak('Charlie', null);

    // This will fail.
    try {
      chrome.experimental.tts.speak('Delta', 'foo');
      chrome.test.fail();
    } catch (e) {
    }

    // This will succeed.
    chrome.experimental.tts.speak('Echo', {}, function() {});

    // This will fail.
    try {
      chrome.experimental.tts.speak('Foxtrot', {}, 'foo');
      chrome.test.fail();
    } catch (e) {
    }

    chrome.test.succeed();
  }
]);
