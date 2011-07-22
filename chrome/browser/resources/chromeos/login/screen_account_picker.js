// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Account picker screen implementation.
 */

cr.define('login', function() {
  /**
   * Creates a new account picker screen div.
   * @constructor
   * @extends {HTMLDivElement}
   */
  var AccountPickerScreen = cr.ui.define('div');

  /**
   * Registers with Oobe.
   */
  AccountPickerScreen.register = function() {
    var screen = $('account-picker');
    AccountPickerScreen.decorate(screen);
    Oobe.getInstance().registerScreen(screen);
  };

  AccountPickerScreen.prototype = {
    __proto__: HTMLDivElement.prototype,

    /** @inheritDoc */
    decorate: function() {
      login.PodRow.decorate($('pod-row'));
    },

    // Whether this screen is shown for the first time.
    firstShown_ : true,

    /**
     * Event handler that is invoked just before the frame is shown.
     * @param data {string} Screen init payload.
     */
    onBeforeShow: function(data) {
      if (this.firstShown_) {
        this.firstShown_ = false;
        $('pod-row').startInitAnimation();
      }
    }
  };

  /**
   * Loads givens users in pod row.
   * @param {array} users Array of user.
   * @public
   */
  AccountPickerScreen.loadUsers = function(users) {
    $('pod-row').loadPods(users);
  };

  return {
    AccountPickerScreen: AccountPickerScreen
  };
});
