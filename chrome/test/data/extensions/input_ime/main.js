// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * This class is a base class of each input method implementation.
 * @constructor
 */
var IMEBase = function() {};
IMEBase.prototype = {
  onActivate: function() {},
  onDeactivated: function() {},
  onFocus: function(context) {},
  onBlur: function(contextID) {},
  onInputContextUpdate: function(context) {},
  onKeyEvent: function(context, engine, keyData) { return false; },
  onCandidateClicked: function(candidateID, button) {},
  onMenuItemActivated: function(name) {}
};

/**
 * This class provides simple identity input methods.
 * @constructor
 **/
var IdentityIME = function() {};
IdentityIME.prototype = new IMEBase();

/**
 * This class provides an IME which capitalize given character.
 * @constructor
 */
var ToUpperIME = function() {};
ToUpperIME.prototype = new IMEBase();

/**
 * @param {Object} context A context object passed from input.ime.onFocus.
 * @param {string} engine An engine ID.
 * @param {Object} keyData A keyevent object passed from input.ime.onKeyEvent.
 * @return {boolean} True on the key event is consumed.
 **/
ToUpperIME.prototype.onKeyEvent = function(context, engine, keyData) {
  if (keyData.type == 'keydown' && /^[a-zA-Z]$/.test(keyData.key)) {
    chrome.input.ime.commitText({
      contextID: context.contextID,
      text: keyData.key.toUpperCase()
    }, function() {});
    return true;
  }
  return false;
};

/**
 * This class listens the event from chrome.input.ime and forwards it to the
 * activated engine.
 * @constructor
 **/
var EngineBridge = function() {};
EngineBridge.prototype = {

  /**
   * Map from engineID to actual engine instance.
   * @type {Object}
   * @private
   **/
  engineInstance_: {},

  /**
   * A current active engineID.
   * @type {string}
   * @private
   **/
  activeEngine_: null,

  /**
   * A input context currently focused.
   * @type {string}
   * @private
   **/
  focusedContext_: null,

  /**
   * Called from chrome.input.ime.onActivate.
   * @private
   * @this EngineBridge
   **/
  onActivate_: function(engineID) {
    this.activeEngine_ = engineID;
    this.engineInstance_[engineID].onActivate();
  },

  /**
   * Called from chrome.input.ime.onDeactivated.
   * @private
   * @this EngineBridge
   **/
  onDeactivated_: function(engineID) {
    if (this.engineInstance_[engineID])
      this.engineInstance_[engineID].onDeactivated();
    this.activeEngine_ = null;
  },

  /**
   * Called from chrome.input.ime.onFocus.
   * @private
   * @this EngineBridge
   **/
  onFocus_: function(context) {
    this.focusedContext_ = context;
    if (this.activeEngine_)
      this.engineInstance_[this.activeEngine_].onFocus(context);
  },

  /**
   * Called from chrome.input.ime.onBlur.
   * @private
   * @this EngineBridge
   **/
  onBlur_: function(contextID) {
    if (this.activeEngine_)
      this.engineInstance_[this.activeEngine_].onBlur(contextID);
    this.focusedContext_ = null;
  },

  /**
   * Called from chrome.input.ime.onInputContextUpdate.
   * @private
   * @this EngineBridge
   **/
  onInputContextUpdate_: function(context) {
    this.focusedContext_ = context;
    if (this.activeEngine_)
      this.engineInstance_[this.activeEngine_].onInputContextUpdate(context);
  },

  /**
   * Called from chrome.input.ime.onKeyEvent.
   * @private
   * @this EngineBridge
   * @return {boolean} True on the key event is consumed.
   **/
  onKeyEvent_: function(engineID, keyData) {
    if (this.engineInstance_[engineID])
      return this.engineInstance_[engineID].onKeyEvent(
          this.focusedContext_, this.activeEngine_, keyData);
    return false;
  },

  /**
   * Called from chrome.input.ime.onCandidateClicked.
   * @private
   * @this EngineBridge
   **/
  onCandidateClicked_: function(engineID, candidateID, button) {
    if (this.engineInstance_[engineID])
      this.engineInstance_[engineID].onCandidateClicked(candidateID, button);
  },

  /**
   * Called from chrome.input.ime.onMenuItemActivated.
   * @private
   * @this EngineBridge
   **/
  onMenuItemActivated_: function(engineID, name) {
    this.engineInstance_[engineID].onMenuItemActivated(name);
  },

  /**
   * Add engine instance for |engineID|.
   * @this EngineBridge
   **/
  addEngine: function(engineID, engine) {
    this.engineInstance_[engineID] = engine;
  },

  /**
   * Initialize EngineBridge by binding with chrome event.
   * @this EngineBridge
   **/
  Initialize: function() {
    chrome.input.ime.onActivate.addListener(this.onActivate_.bind(this));
    chrome.input.ime.onDeactivated.addListener(this.onDeactivated_.bind(this));
    chrome.input.ime.onFocus.addListener(this.onFocus_.bind(this));
    chrome.input.ime.onBlur.addListener(this.onBlur_.bind(this));
    chrome.input.ime.onInputContextUpdate.addListener(
        this.onInputContextUpdate_.bind(this));
    chrome.input.ime.onKeyEvent.addListener(this. onKeyEvent_.bind(this));
    chrome.input.ime.onCandidateClicked.addListener(
        this.onCandidateClicked_.bind(this));
    chrome.input.ime.onMenuItemActivated.addListener(
        this.onMenuItemActivated_.bind(this));
  }
};

document.addEventListener('readystatechange', function() {
  if (document.readyState === 'complete') {
    var engineBridge = new EngineBridge();
    engineBridge.Initialize();
    engineBridge.addEngine('IdentityIME', new IdentityIME());
    engineBridge.addEngine('ToUpperIME', new ToUpperIME());
  }
});
