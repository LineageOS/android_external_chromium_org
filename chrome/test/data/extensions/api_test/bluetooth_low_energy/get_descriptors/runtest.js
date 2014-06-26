// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testGetDescriptors() {
  chrome.test.assertEq(2, descrs.length);

  chrome.test.assertEq('desc_id0', descrs[0].instanceId);
  chrome.test.assertEq('00001221-0000-1000-8000-00805f9b34fb', descrs[0].uuid);
  chrome.test.assertEq(false, descrs[0].isLocal);
  chrome.test.assertEq(charId, descrs[0].characteristic.instanceId);

  var valueBytes = new Uint8Array(descrs[0].value);
  chrome.test.assertEq(3, descrs[0].value.byteLength);
  chrome.test.assertEq(0x01, valueBytes[0]);
  chrome.test.assertEq(0x02, valueBytes[1]);
  chrome.test.assertEq(0x03, valueBytes[2]);

  chrome.test.assertEq('desc_id1', descrs[1].instanceId);
  chrome.test.assertEq('00001222-0000-1000-8000-00805f9b34fb', descrs[1].uuid);
  chrome.test.assertEq(false, descrs[1].isLocal);
  chrome.test.assertEq(charId, descrs[1].characteristic.instanceId);

  valueBytes = new Uint8Array(descrs[1].value);
  chrome.test.assertEq(2, descrs[1].value.byteLength);
  chrome.test.assertEq(0x04, valueBytes[0]);
  chrome.test.assertEq(0x05, valueBytes[1]);

  chrome.test.succeed();
}

var getDescriptors = chrome.bluetoothLowEnergy.getDescriptors;
var charId = 'char_id0';
var badCharId = 'char_id1';

var descrs = null;

function failOnError() {
  if (chrome.runtime.lastError) {
    chrome.test.fail(chrome.runtime.lastError.message);
  }
}

// 1. Unknown characteristic ID.
getDescriptors(badCharId, function (result) {
  if (result || !chrome.runtime.lastError) {
    chrome.test.fail('getDescriptors should have failed for \'badCharId\'');
  }

  // 2. Known ID, unknown characteristic.
  getDescriptors(charId, function (result) {
    if (result || !chrome.runtime.lastError) {
      chrome.test.fail('getDescriptors should have failed');
    }

    // 3. Empty descriptors.
    getDescriptors(charId, function (result) {
      failOnError();
      if (!result || result.length != 0) {
        chrome.test.fail('Descriptors should be empty');
      }

      // 4. Success.
      getDescriptors(charId, function (result) {
        failOnError();
        descrs = result;

        chrome.test.sendMessage('ready', function (message) {
          chrome.test.runTests([testGetDescriptors]);
        });
      });
    });
  });
});
