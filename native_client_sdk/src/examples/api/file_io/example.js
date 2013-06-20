// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function moduleDidLoad() {
  common.hideModule();
}

// Called by the common.js module.
function domContentLoaded(name, tc, config, width, height) {
  navigator.webkitPersistentStorage.requestQuota(1024 * 1024,
    function(bytes) {
      common.updateStatus(
          'Allocated ' + bytes + ' bytes of persistant storage.');
      common.createNaClModule(name, tc, config, width, height);
      common.attachDefaultListeners();
    },
    function(e) { alert('Failed to allocate space') });
}

// Called by the common.js module.
function attachListeners() {
  var radioEls = document.querySelectorAll('input[type="radio"]');
  for (var i = 0; i < radioEls.length; ++i) {
    radioEls[i].addEventListener('click', onRadioClicked);
  }

  function addEventListenerToButton(parentId, func) {
    document.querySelector('#' + parentId + ' button')
        .addEventListener('click', func);
  }

  addEventListenerToButton('saveFile', saveFile);
  addEventListenerToButton('loadFile', loadFile);
  addEventListenerToButton('delete', deleteFileOrDirectory);
  addEventListenerToButton('listDir', listDir);
  addEventListenerToButton('makeDir', makeDir);
}

function onRadioClicked(e) {
  var divId = this.id.slice(6);  // skip "radio_"
  var functionEls = document.querySelectorAll('.function');
  for (var i = 0; i < functionEls.length; ++i) {
    var visible = functionEls[i].id === divId;
    if (functionEls[i].id === divId)
      functionEls[i].removeAttribute('hidden');
    else
      functionEls[i].setAttribute('hidden');
  }
}

function makeMessage(command, path) {
  // Package a message using a simple protocol containing:
  // command <path length> <path> <space-separated extra args>
  var msg = command;
  msg += ' ';
  msg += path.length;
  msg += ' ';
  msg += path;
  // Maybe add extra args
  for (var i = 2; i < arguments.length; ++i) {
    msg += ' ' + arguments[i];
  }
  return msg;
}

function saveFile() {
  if (common.naclModule) {
    var fileName = document.querySelector('#saveFile input').value;
    var fileText = document.querySelector('#saveFile textarea').value;
    common.naclModule.postMessage(makeMessage('sv', fileName, fileText));
    // clear the editor.
    fileText.value = '';
  }
}

function loadFile() {
  if (common.naclModule) {
    var fileName = document.querySelector('#loadFile input').value;
    // clear the editor first (in case there is an error and there is no
    // output).
    document.querySelector('#loadFile textarea').value = '';
    common.naclModule.postMessage(makeMessage('ld', fileName));
  }
}

function deleteFileOrDirectory() {
  if (common.naclModule) {
    var fileName = document.querySelector('#delete input').value;
    common.naclModule.postMessage(makeMessage('de', fileName));
  }
}

function listDir() {
  if (common.naclModule) {
    var dirName = document.querySelector('#listDir input').value;
    common.naclModule.postMessage(makeMessage('ls', dirName));
  }
}

function makeDir() {
  if (common.naclModule) {
    var dirName = document.querySelector('#makeDir input').value;
    common.naclModule.postMessage(makeMessage('md', dirName));
  }
}

// Called by the common.js module.
function handleMessage(message_event) {
  var msg = message_event.data;
  var parts = msg.split('|');
  var command = parts[0];
  var args = parts.slice(1);

  if (command == 'ERR') {
    common.logMessage('Error: ' + args[0] + '\n');
  } else if (command == 'STAT') {
    common.logMessage(args[0] + '\n');
  } else if (command == 'READY') {
    common.logMessage('Filesystem ready!\n');
  } else if (command == 'DISP') {
    // Find the file editor that is currently visible.
    var fileEditorEl =
        document.querySelector('.function:not([hidden]) > textarea');
    // Rejoin args with pipe (|) -- there is only one argument, and it can
    // contain the pipe character.
    fileEditorEl.value = args.join('|');
  } else if (command == 'LIST') {
    var listDirOutputEl = document.getElementById('listDirOutput');

    // NOTE: files with | in their names will be incorrectly split. Fixing this
    // is left as an exercise for the reader.

    // Remove all children of this element...
    while (listDirOutputEl.firstChild) {
      listDirOutputEl.removeChild(listDirOutputEl.firstChild);
    }

    if (args.length) {
      // Add new <li> elements for each file.
      for (var i = 0; i < args.length; ++i) {
        var itemEl = document.createElement('li');
        itemEl.textContent = args[i];
        listDirOutputEl.appendChild(itemEl);
      }
    } else {
      var itemEl = document.createElement('li');
      itemEl.textContent = '<empty directory>';
      listDirOutputEl.appendChild(itemEl);
    }
  }
}
