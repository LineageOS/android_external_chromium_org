// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var fileSystemNatives = requireNative('file_system_natives');

var nameToIds = {};
var idsToEntries = {};

function computeName(entry) {
  return entry.filesystem.name + ':' + entry.fullPath;
}

function computeId(entry) {
  var fileSystemId = fileSystemNatives.CrackIsolatedFileSystemName(
      entry.filesystem.name);
  if (!fileSystemId)
    return null;
  // Strip the leading '/' from the path.
  return fileSystemId + ':' + entry.fullPath.slice(1);
}

function registerEntry(id, entry) {
  var name = computeName(entry);
  nameToIds[name] = id;
  idsToEntries[id] = entry;
}

function getEntryId(entry) {
  var name = null;
  try {
    name = computeName(entry);
  } catch(e) {
    return null;
  }
  var id = nameToIds[name];
  if (id != null)
    return id;

  // If an entry has not been registered, compute its id and register it.
  id = computeId(entry);
  registerEntry(id, entry);
  return id;
}

function getEntryById(id) {
  return idsToEntries[id];
}

exports.registerEntry = registerEntry;
exports.getEntryId = getEntryId;
exports.getEntryById = getEntryById;
