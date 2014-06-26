// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A simple container object for the brailling of a
 * navigation from one object to another.
 *
 */


goog.provide('cvox.NavBraille');

goog.require('cvox.ChromeVox');
goog.require('cvox.CursorSelection');
goog.require('cvox.Spannable');

/**
 * A class capturing the braille for navigation from one object to
 * another.
 * @param {{text: (undefined|string|!cvox.Spannable),
 *          startIndex: (undefined|number),
 *          endIndex: (undefined|number)}} kwargs The arguments for braille.
 *  text The text of the object itself, including text from
 *     titles, labels, etc.
 *  startIndex The beginning of a selection within text.
 *  endIndex The end of a selection within text.
 * @constructor
 */
cvox.NavBraille = function(kwargs) {
  /**
   * Text, annotated with DOM nodes.
   * @type {!cvox.Spannable}
   */
  this.text = (kwargs.text instanceof cvox.Spannable) ?
      kwargs.text : new cvox.Spannable(kwargs.text);

  /**
   * Selection start index.
   * @type {number}
   */
  this.startIndex = goog.isDef(kwargs.startIndex) ? kwargs.startIndex : -1;

  /**
   * Selection end index.
   * @type {number}
   */
  this.endIndex = goog.isDef(kwargs.endIndex) ?
      kwargs.endIndex : this.startIndex;
};

/**
 * Convenience for creating simple braille output.
 * @param {string|!cvox.Spannable} text Text to represent in braille.
 * @return {!cvox.NavBraille} Braille output without a cursor.
 */
cvox.NavBraille.fromText = function(text) {
  return new cvox.NavBraille({'text': text});
};


/**
 * Creates a NavBraille from its serialized json form as created
 * by toJson().
 * @param {!Object} json the serialized json object.
 * @return {!cvox.NavBraille}
 */
cvox.NavBraille.fromJson = function(json) {
  if (typeof json.startIndex !== 'number' ||
      typeof json.endIndex !== 'number') {
    throw 'Invalid start or end index in serialized NavBraille: ' + json;
  }
  return new cvox.NavBraille({
    text: cvox.Spannable.fromJson(json.spannable),
    startIndex: json.startIndex,
    endIndex: json.endIndex
  });
};


/**
 * @return {boolean} true if this braille description is empty.
 */
cvox.NavBraille.prototype.isEmpty = function() {
  return this.text.getLength() == 0;
};


/**
 * @return {string} A string representation of this object.
 */
cvox.NavBraille.prototype.toString = function() {
  return 'NavBraille(text="' + this.text.toString() + '" ' +
         ' startIndex="' + this.startIndex + '" ' +
         ' endIndex="' + this.endIndex + '")';
};


/**
 * Returns a plain old data object with the same data.
 * Suitable for JSON encoding.
 *
 * @return {{spannable: Object,
 *           startIndex: number,
 *           endIndex: number}} JSON equivalent.
 */
cvox.NavBraille.prototype.toJson = function() {
  return {
    spannable: this.text.toJson(),
    startIndex: this.startIndex,
    endIndex: this.endIndex
  };
};


/**
 * Sends braille to the background page.
 */
cvox.NavBraille.prototype.write = function() {
  cvox.ChromeVox.braille.write(this);
};
