// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * Get the url relative to the main extension url. If the url is
   * unassociated with the extension, this will be the full url.
   * @param {string} url The url to make relative.
   * @param {string} extensionUrl The url for the extension resources, in the
   *     form "chrome-etxension://<extension_id>/".
   * @return {string} The url relative to the host.
   */
  function getRelativeUrl(url, extensionUrl) {
    return url.substring(0, extensionUrl.length) == extensionUrl ?
        url.substring(extensionUrl.length) : url;
  }

  /**
   * The RuntimeErrorContent manages all content specifically associated with
   * runtime errors; this includes stack frames and the context url.
   * @constructor
   * @extends {HTMLDivElement}
   */
  function RuntimeErrorContent() {
    var contentArea = $('template-collection-extension-error-overlay').
        querySelector('.extension-error-overlay-runtime-content').
        cloneNode(true);
    contentArea.__proto__ = RuntimeErrorContent.prototype;
    contentArea.init();
    return contentArea;
  }

  /**
   * The name of the "active" class specific to extension errors (so as to
   * not conflict with other rules).
   * @type {string}
   * @const
   */
  RuntimeErrorContent.ACTIVE_CLASS_NAME = 'extension-error-active';

  /**
   * Determine whether or not we should display the url to the user. We don't
   * want to include any of our own code in stack traces.
   * @param {string} url The url in question.
   * @return {boolean} True if the url should be displayed, and false
   *     otherwise (i.e., if it is an internal script).
   */
  RuntimeErrorContent.shouldDisplayForUrl = function(url) {
    // All our internal scripts are in the 'extensions::' namespace.
    return !/^extensions::/.test(url);
  };

  /**
   * Send a call to chrome to open the developer tools for an error.
   * This will call either the bound function in ExtensionErrorHandler or the
   * API function from developerPrivate, depending on whether this is being
   * used in the native chrome:extensions page or the Apps Developer Tool.
   * @see chrome/browser/ui/webui/extensions/extension_error_ui_util.h
   * @param {Object} args The arguments to pass to openDevTools.
   * @private
   */
  RuntimeErrorContent.openDevtools_ = function(args) {
    if (chrome.send)
      chrome.send('extensionErrorOpenDevTools', [args]);
    else if (chrome.developerPrivate)
      chrome.developerPrivate.openDevTools(args);
    else
      assert(false, 'Cannot call either openDevTools function.');
  };

  RuntimeErrorContent.prototype = {
    __proto__: HTMLDivElement.prototype,

    /**
     * The underlying error whose details are being displayed.
     * @type {Object}
     * @private
     */
    error_: undefined,

    /**
     * The URL associated with this extension, i.e. chrome-extension://<id>/.
     * @type {string}
     * @private
     */
    extensionUrl_: undefined,

    /**
     * The node of the stack trace which is currently active.
     * @type {HTMLElement}
     * @private
     */
    currentFrameNode_: undefined,

    /**
     * Initialize the RuntimeErrorContent for the first time.
     */
    init: function() {
      /**
       * The stack trace element in the overlay.
       * @type {HTMLElement}
       * @private
       */
      this.stackTrace_ =
          this.querySelector('.extension-error-overlay-stack-trace-list');
      assert(this.stackTrace_);

      /**
       * The context URL element in the overlay.
       * @type {HTMLElement}
       * @private
       */
      this.contextUrl_ =
          this.querySelector('.extension-error-overlay-context-url');
      assert(this.contextUrl_);
    },

    /**
     * Sets the error for the content.
     * @param {Object} error The error whose content should be displayed.
     * @param {string} extensionUrl The URL associated with this extension.
     */
    setError: function(error, extensionUrl) {
      this.error_ = error;
      this.extensionUrl_ = extensionUrl;
      this.contextUrl_.textContent = error.contextUrl ?
          getRelativeUrl(error.contextUrl, this.extensionUrl_) :
          loadTimeData.getString('extensionErrorOverlayContextUnknown');
      this.initStackTrace_();
    },

    /**
     * Wipe content associated with a specific error.
     */
    clearError: function() {
      this.error_ = undefined;
      this.extensionUrl_ = undefined;
      this.currentFrameNode_ = undefined;
      this.stackTrace_.innerHTML = '';
      this.stackTrace_.hidden = true;
    },

    /**
     * Makes |frame| active and deactivates the previously active frame (if
     * there was one).
     * @param {HTMLElement} frame The frame to activate.
     * @private
     */
    setActiveFrame_: function(frameNode) {
      if (this.currentFrameNode_) {
        this.currentFrameNode_.classList.remove(
            RuntimeErrorContent.ACTIVE_CLASS_NAME);
      }

      this.currentFrameNode_ = frameNode;
      this.currentFrameNode_.classList.add(
          RuntimeErrorContent.ACTIVE_CLASS_NAME);
    },

    /**
     * Initialize the stack trace element of the overlay.
     * @private
     */
    initStackTrace_: function() {
      for (var i = 0; i < this.error_.stackTrace.length; ++i) {
        var frame = this.error_.stackTrace[i];
        // Don't include any internal calls (e.g., schemaBindings) in the
        // stack trace.
        if (!RuntimeErrorContent.shouldDisplayForUrl(frame.url))
          continue;

        var frameNode = document.createElement('li');
        // Attach the index of the frame to which this node refers (since we
        // may skip some, this isn't a 1-to-1 match).
        frameNode.indexIntoTrace = i;

        // The description is a human-readable summation of the frame, in the
        // form "<relative_url>:<line_number> (function)", e.g.
        // "myfile.js:25 (myFunction)".
        var description = getRelativeUrl(frame.url, this.extensionUrl_) +
                          ':' + frame.lineNumber;
        if (frame.functionName) {
          var functionName = frame.functionName == '(anonymous function)' ?
              loadTimeData.getString('extensionErrorOverlayAnonymousFunction') :
              frame.functionName;
          description += ' (' + functionName + ')';
        }
        frameNode.textContent = description;

        // When the user clicks on a frame in the stack trace, we should
        // highlight that overlay in the list, display the appropriate source
        // code with the line highlighted, and link the "Open DevTools" button
        // with that frame.
        frameNode.addEventListener('click', function(frame, frameNode, e) {
          if (this.currStackFrame_ == frameNode)
            return;

          this.setActiveFrame_(frameNode);

          // Request the file source with the section highlighted; this will
          // call ExtensionErrorOverlay.requestFileSourceResponse() when
          // completed, which in turn calls setCode().
          ExtensionErrorOverlay.requestFileSource(
              {extensionId: this.error_.extensionId,
               message: this.error_.message,
               pathSuffix: getRelativeUrl(frame.url, this.extensionUrl_),
               lineNumber: frame.lineNumber});
        }.bind(this, frame, frameNode));

        this.stackTrace_.appendChild(frameNode);
      }

      // Set the current stack frame to the first stack frame and show the
      // trace, if one exists. (We can't just check error.stackTrace, because
      // it's possible the trace was purely internal, and we don't show
      // internal frames.)
      if (this.stackTrace_.children.length > 0) {
        this.stackTrace_.hidden = false;
        this.setActiveFrame_(this.stackTrace_.firstChild);
      }
    },

    /**
     * Open the developer tools for the active stack frame.
     */
    openDevtools: function() {
      var stackFrame =
          this.error_.stackTrace[this.currentFrameNode_.indexIntoTrace];

      RuntimeErrorContent.openDevtools_(
          {renderProcessId: this.error_.renderProcessId,
           renderViewId: this.error_.renderViewId,
           url: stackFrame.url,
           lineNumber: stackFrame.lineNumber || 0,
           columnNumber: stackFrame.columnNumber || 0});
    }
  };

  /**
   * The ExtensionErrorOverlay will show the contents of a file which pertains
   * to the ExtensionError; this is either the manifest file (for manifest
   * errors) or a source file (for runtime errors). If possible, the portion
   * of the file which caused the error will be highlighted.
   * @constructor
   */
  function ExtensionErrorOverlay() {
    /**
     * The content section for runtime errors; this is re-used for all
     * runtime errors and attached/detached from the overlay as needed.
     * @type {RuntimeErrorContent}
     * @private
     */
    this.runtimeErrorContent_ = new RuntimeErrorContent();
  }

  /**
   * Value of ExtensionError::RUNTIME_ERROR enum.
   * @see extensions/browser/extension_error.h
   * @type {number}
   * @const
   * @private
   */
  ExtensionErrorOverlay.RUNTIME_ERROR_TYPE_ = 1;

  /**
   * The manifest filename.
   * @type {string}
   * @const
   * @private
   */
  ExtensionErrorOverlay.MANIFEST_FILENAME_ = 'manifest.json';

  /**
   * Determine whether or not chrome can load the source for a given file; this
   * can only be done if the file belongs to the extension.
   * @param {string} file The file to load.
   * @param {string} extensionUrl The url for the extension, in the form
   *     chrome-extension://<extension-id>/.
   * @return {boolean} True if the file can be loaded, false otherwise.
   * @private
   */
  ExtensionErrorOverlay.canLoadFileSource = function(file, extensionUrl) {
    return file.substr(0, extensionUrl.length) == extensionUrl ||
           file.toLowerCase() == ExtensionErrorOverlay.MANIFEST_FILENAME_;
  };

  /**
   * Determine whether or not we can show an overlay with more details for
   * the given extension error.
   * @param {Object} error The extension error.
   * @param {string} extensionUrl The url for the extension, in the form
   *     "chrome-extension://<extension-id>/".
   * @return {boolean} True if we can show an overlay for the error,
   *     false otherwise.
   */
  ExtensionErrorOverlay.canShowOverlayForError = function(error, extensionUrl) {
    if (ExtensionErrorOverlay.canLoadFileSource(error.source, extensionUrl))
      return true;

    if (error.stackTrace) {
      for (var i = 0; i < error.stackTrace.length; ++i) {
        if (RuntimeErrorContent.shouldDisplayForUrl(error.stackTrace[i].url))
          return true;
      }
    }

    return false;
  };

  /**
   * Send a call to chrome to request the source of a given file.
   * This will call either the bound function in ExtensionErrorHandler or the
   * API function from developerPrivate, depending on whether this is being
   * used in the native chrome:extensions page or the Apps Developer Tool.
   * @see chrome/browser/ui/webui/extensions/extension_error_ui_util.h
   * @param {Object} args The arguments to pass to requestFileSource.
   */
  ExtensionErrorOverlay.requestFileSource = function(args) {
    if (chrome.send) {
      chrome.send('extensionErrorRequestFileSource', [args]);
    } else if (chrome.developerPrivate) {
      chrome.developerPrivate.requestFileSource(args, function(result) {
        extensions.ExtensionErrorOverlay.requestFileSourceResponse(result);
      });
    } else {
      assert(false, 'Cannot call either requestFileSource function.');
    }
  };

  cr.addSingletonGetter(ExtensionErrorOverlay);

  ExtensionErrorOverlay.prototype = {
    /**
     * The underlying error whose details are being displayed.
     * @type {Object}
     * @private
     */
    error_: undefined,

    /**
     * Initialize the page.
     * @param {function(HTMLDivElement)} showOverlay The function to show or
     *     hide the ExtensionErrorOverlay; this should take a single parameter
     *     which is either the overlay Div if the overlay should be displayed,
     *     or null if the overlay should be hidden.
     */
    initializePage: function(showOverlay) {
      var overlay = $('overlay');
      cr.ui.overlay.setupOverlay(overlay);
      cr.ui.overlay.globalInitialization();
      overlay.addEventListener('cancelOverlay', this.handleDismiss_.bind(this));

      $('extension-error-overlay-dismiss').addEventListener(
          'click', this.handleDismiss_.bind(this));

      /**
       * The element of the full overlay.
       * @type {HTMLDivElement}
       * @private
       */
      this.overlayDiv_ = $('extension-error-overlay');

      /**
       * The portion of the overlay which shows the code relating to the error.
       * @type {HTMLElement}
       * @private
       */
      this.codeDiv_ = $('extension-error-overlay-code');

      /**
       * The function to show or hide the ExtensionErrorOverlay.
       * @type {function}
       * @param {boolean} isVisible Whether the overlay should be visible.
       */
      this.setVisible = function(isVisible) {
        showOverlay(isVisible ? this.overlayDiv_ : null);
      };

      /**
       * The button to open the developer tools (only available for runtime
       * errors).
       * @type {HTMLButtonElement}
       * @private
       */
      this.openDevtoolsButton_ = $('extension-error-overlay-devtools-button');
      this.openDevtoolsButton_.addEventListener('click', function() {
          this.runtimeErrorContent_.openDevtools();
      }.bind(this));
    },

    /**
     * Handles a click on the dismiss ("OK" or close) buttons.
     * @param {Event} e The click event.
     * @private
     */
    handleDismiss_: function(e) {
      this.setVisible(false);

      // There's a chance that the overlay receives multiple dismiss events; in
      // this case, handle it gracefully and return (since all necessary work
      // will already have been done).
      if (!this.error_)
        return;

      this.codeDiv_.innerHTML = '';
      this.openDevtoolsButton_.hidden = true;

      if (this.error_.type == ExtensionErrorOverlay.RUNTIME_ERROR_TYPE_) {
        this.overlayDiv_.querySelector('.content-area').removeChild(
            this.runtimeErrorContent_);
        this.runtimeErrorContent_.clearError();
      }

      this.error_ = undefined;
    },

    /**
     * Associate an error with the overlay. This will set the error for the
     * overlay, and, if possible, will populate the code section of the overlay
     * with the relevant file, load the stack trace, and generate links for
     * opening devtools (the latter two only happen for runtime errors).
     * @param {Object} error The error to show in the overlay.
     * @param {string} extensionUrl The URL of the extension, in the form
     *     "chrome-extension://<extension_id>".
     */
    setErrorAndShowOverlay: function(error, extensionUrl) {
      this.error_ = error;

      if (this.error_.type == ExtensionErrorOverlay.RUNTIME_ERROR_TYPE_) {
        this.runtimeErrorContent_.setError(this.error_, extensionUrl);
        this.overlayDiv_.querySelector('.content-area').insertBefore(
            this.runtimeErrorContent_,
            this.codeDiv_.nextSibling);
        this.openDevtoolsButton_.hidden = false;
        this.openDevtoolsButton_.disabled = !error.canInspect;
      }

      if (ExtensionErrorOverlay.canLoadFileSource(error.source, extensionUrl)) {
        var relativeUrl = getRelativeUrl(error.source, extensionUrl);

        var requestFileSourceArgs = {extensionId: error.extensionId,
                                     message: error.message,
                                     pathSuffix: relativeUrl};

        if (relativeUrl.toLowerCase() ==
                ExtensionErrorOverlay.MANIFEST_FILENAME_) {
          requestFileSourceArgs.manifestKey = error.manifestKey;
          requestFileSourceArgs.manifestSpecific = error.manifestSpecific;
        } else {
          requestFileSourceArgs.lineNumber =
              error.stackTrace && error.stackTrace[0] ?
                  error.stackTrace[0].lineNumber : 0;
        }
        ExtensionErrorOverlay.requestFileSource(requestFileSourceArgs);
      } else {
        ExtensionErrorOverlay.requestFileSourceResponse(null);
      }
    },

    /**
     * Set the code to be displayed in the code portion of the overlay.
     * @see ExtensionErrorOverlay.requestFileSourceResponse().
     * @param {?Object} code The code to be displayed. If |code| is null, then
     *     a "Could not display code" message will be displayed instead.
     */
    setCode: function(code) {
      document.querySelector(
          '#extension-error-overlay .extension-error-overlay-title').
              textContent = code.title;
      this.codeDiv_.innerHTML = '';

      // If there's no code, then display an appropriate message.
      if (!code) {
        var span = document.createElement('span');
        span.textContent =
            loadTimeData.getString('extensionErrorOverlayNoCodeToDisplay');
        this.codeDiv_.appendChild(span);
        return;
      }

      var createSpan = function(source, isHighlighted) {
        var span = document.createElement('span');
        span.className = isHighlighted ? 'highlighted-source' : 'normal-source';
        source = source.replace(/ /g, '&nbsp;').replace(/\n|\r/g, '<br>');
        span.innerHTML = source;
        return span;
      };

      if (code.beforeHighlight)
        this.codeDiv_.appendChild(createSpan(code.beforeHighlight, false));

      if (code.highlight) {
        var highlightSpan = createSpan(code.highlight, true);
        highlightSpan.title = code.message;
        this.codeDiv_.appendChild(highlightSpan);
      }

      if (code.afterHighlight)
        this.codeDiv_.appendChild(createSpan(code.afterHighlight, false));
    },
  };

  /**
   * Called by the ExtensionErrorHandler responding to the request for a file's
   * source. Populate the content area of the overlay and display the overlay.
   * @param {Object?} result An object with four strings - the title,
   *     beforeHighlight, afterHighlight, and highlight. The three 'highlight'
   *     strings represent three portions of the file's content to display - the
   *     portion which is most relevant and should be emphasized (highlight),
   *     and the parts both before and after this portion. These may be empty.
   */
  ExtensionErrorOverlay.requestFileSourceResponse = function(result) {
    var overlay = extensions.ExtensionErrorOverlay.getInstance();
    overlay.setCode(result);
    overlay.setVisible(true);
  };

  // Export
  return {
    ExtensionErrorOverlay: ExtensionErrorOverlay
  };
});
