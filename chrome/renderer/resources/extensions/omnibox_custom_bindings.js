// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom bindings for the omnibox API. Only injected into the v8 contexts
// for extensions which have permission for the omnibox API.

var chromeHidden = requireNative('chrome_hidden').GetChromeHidden();
var sendRequest = require('sendRequest').sendRequest;

// Remove invalid characters from |text| so that it is suitable to use
// for |AutocompleteMatch::contents|.
function sanitizeString(text, shouldTrim) {
  // NOTE: This logic mirrors |AutocompleteMatch::SanitizeString()|.
  // 0x2028 = line separator; 0x2029 = paragraph separator.
  var kRemoveChars = /(\r|\n|\t|\u2028|\u2029)/gm;
  if (shouldTrim)
    text = text.trimLeft();
  return text.replace(kRemoveChars, '');
}

// Parses the xml syntax supported by omnibox suggestion results. Returns an
// object with two properties: 'description', which is just the text content,
// and 'descriptionStyles', which is an array of style objects in a format
// understood by the C++ backend.
function parseOmniboxDescription(input) {
  var domParser = new DOMParser();

  // The XML parser requires a single top-level element, but we want to
  // support things like 'hello, <match>world</match>!'. So we wrap the
  // provided text in generated root level element.
  var root = domParser.parseFromString(
      '<fragment>' + input + '</fragment>', 'text/xml');

  // DOMParser has a terrible error reporting facility. Errors come out nested
  // inside the returned document.
  var error = root.querySelector('parsererror div');
  if (error) {
    throw new Error(error.textContent);
  }

  // Otherwise, it's valid, so build up the result.
  var result = {
    description: '',
    descriptionStyles: []
  };

  // Recursively walk the tree.
  (function(node) {
    for (var i = 0, child; child = node.childNodes[i]; i++) {
      // Append text nodes to our description.
      if (child.nodeType == Node.TEXT_NODE) {
        var shouldTrim = result.description.length == 0;
        result.description += sanitizeString(child.nodeValue, shouldTrim);
        continue;
      }

      // Process and descend into a subset of recognized tags.
      if (child.nodeType == Node.ELEMENT_NODE &&
          (child.nodeName == 'dim' || child.nodeName == 'match' ||
           child.nodeName == 'url')) {
        var style = {
          'type': child.nodeName,
          'offset': result.description.length
        };
        result.descriptionStyles.push(style);
        arguments.callee(child);
        style.length = result.description.length - style.offset;
        continue;
      }

      // Descend into all other nodes, even if they are unrecognized, for
      // forward compat.
      arguments.callee(child);
    }
  })(root);

  return result;
}

chromeHidden.registerCustomHook('omnibox', function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;

  apiFunctions.setHandleRequest('setDefaultSuggestion', function(details) {
    var parseResult = parseOmniboxDescription(details.description);
    sendRequest(this.name, [parseResult], this.definition.parameters);
  });

  apiFunctions.setUpdateArgumentsPostValidate(
      'sendSuggestions', function(requestId, userSuggestions) {
    var suggestions = [];
    for (var i = 0; i < userSuggestions.length; i++) {
      var parseResult = parseOmniboxDescription(
          userSuggestions[i].description);
      parseResult.content = userSuggestions[i].content;
      suggestions.push(parseResult);
    }
    return [requestId, suggestions];
  });

  chrome.omnibox.onInputChanged.dispatch =
      function(text, requestId) {
    var suggestCallback = function(suggestions) {
      chrome.omnibox.sendSuggestions(requestId, suggestions);
    };
    chrome.Event.prototype.dispatch.apply(this, [text, suggestCallback]);
  };
});
