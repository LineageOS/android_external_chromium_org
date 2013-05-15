// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Namespace for async utility functions.
 */
var AsyncUtil = {};

/**
 * Creates a class for executing several asynchronous closures in a fifo queue.
 * Added tasks will be executed sequentially in order they were added.
 *
 * @constructor
 */
AsyncUtil.Queue = function() {
  this.running_ = false;
  this.closures_ = [];
};

/**
 * Enqueues a closure to be executed.
 * @param {function(function())} closure Closure with a completion callback to
 *     be executed.
 */
AsyncUtil.Queue.prototype.run = function(closure) {
  this.closures_.push(closure);
  if (!this.running_)
    this.continue_();
};

/**
 * Serves the next closure from the queue.
 * @private
 */
AsyncUtil.Queue.prototype.continue_ = function() {
  if (!this.closures_.length) {
    this.running_ = false;
    return;
  }

  // Run the next closure.
  this.running_ = true;
  var closure = this.closures_.shift();
  closure(this.continue_.bind(this));
};

/**
 * Creates a class for executing several asynchronous closures in a group in
 * a dependency order.
 *
 * @constructor
 */
AsyncUtil.Group = function() {
  this.addedTasks_ = {};
  this.pendingTasks_ = {};
  this.finishedTasks_ = {};
  this.completionCallbacks_ = [];
};

/**
 * Enqueues a closure to be executed after dependencies are completed.
 *
 * @param {function(function())} closure Closure with a completion callback to
 *     be executed.
 * @param {Array.<string>=} opt_dependencies Array of dependencies. If no
 *     dependencies, then the the closure will be executed immediately.
 * @param {string=} opt_name Task identifier. Specify to use in dependencies.
 */
AsyncUtil.Group.prototype.add = function(closure, opt_dependencies, opt_name) {
  var length = Object.keys(this.addedTasks_).length;
  var name = opt_name || ('(unnamed#' + (length + 1) + ')');

  var task = {
    closure: closure,
    dependencies: opt_dependencies || [],
    name: name
  };

  this.addedTasks_[name] = task;
  this.pendingTasks_[name] = task;
};

/**
 * Runs the enqueued closured in order of dependencies.
 *
 * @param {function()=} opt_onCompletion Completion callback.
 */
AsyncUtil.Group.prototype.run = function(opt_onCompletion) {
  if (opt_onCompletion)
    this.completionCallbacks_.push(opt_onCompletion);
  this.continue_();
};

/**
 * Runs enqueued pending tasks whose dependencies are completed.
 * @private
 */
AsyncUtil.Group.prototype.continue_ = function() {
  // If all of the added tasks have finished, then call completion callbacks.
  if (Object.keys(this.addedTasks_).length ==
      Object.keys(this.finishedTasks_).length) {
    for (var index = 0; index < this.completionCallbacks_.length; index++) {
      var callback = this.completionCallbacks_[index];
      callback();
    }
    this.completionCallbacks_ = [];
    return;
  }

  for (var name in this.pendingTasks_) {
    var task = this.pendingTasks_[name];
    var dependencyMissing = false;
    for (var index = 0; index < task.dependencies.length; index++) {
      var dependency = task.dependencies[index];
      // Check if the dependency has finished.
      if (!this.finishedTasks_[dependency])
        dependencyMissing = true;
    }
    // All dependences finished, therefore start the task.
    if (!dependencyMissing) {
      delete this.pendingTasks_[task.name];
      task.closure(this.finish_.bind(this, task));
    }
  }
};

/**
 * Finishes the passed task and continues executing enqueued closures.
 *
 * @param {Object} task Task object.
 * @private
 */
AsyncUtil.Group.prototype.finish_ = function(task) {
  this.finishedTasks_[task.name] = task;
  this.continue_();
};
