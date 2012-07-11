// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_MANAGER_H_

#include <map>
#include <string>

#include "base/logging.h"  // for NOTIMPLEMENTED()
#include "chrome/browser/chromeos/input_method/input_method_config.h"
#include "chrome/browser/chromeos/input_method/input_method_descriptor.h"
#include "chrome/browser/chromeos/input_method/input_method_property.h"
#include "chrome/browser/chromeos/input_method/input_method_util.h"

namespace ui {
class Accelerator;
}  // namespace ui

namespace chromeos {
namespace input_method {

class XKeyboard;

// This class manages input methodshandles.  Classes can add themselves as
// observers. Clients can get an instance of this library class by:
// InputMethodManager::GetInstance().
class InputMethodManager {
 public:
  enum State {
    STATE_LOGIN_SCREEN = 0,
    STATE_BROWSER_SCREEN,
    STATE_LOCK_SCREEN,
    STATE_TERMINATING,
  };

  class Observer {
   public:
    virtual ~Observer() {}
    // Called when the current input method is changed.  |show_message|
    // indicates whether the user should be notified of this change.
    virtual void InputMethodChanged(InputMethodManager* manager,
                                    bool show_message) = 0;
    // Called when the list of properties is changed.
    virtual void InputMethodPropertyChanged(InputMethodManager* manager) = 0;
  };

  // CandidateWindowObserver is notified of events related to the candidate
  // window.  The "suggestion window" used by IMEs such as ibus-mozc does not
  // count as the candidate window (this may change if we later want suggestion
  // window events as well).  These events also won't occur when the virtual
  // keyboard is used, since it controls its own candidate window.
  class CandidateWindowObserver {
   public:
    virtual ~CandidateWindowObserver() {}
    // Called when the candidate window is opened.
    virtual void CandidateWindowOpened(InputMethodManager* manager) = 0;
    // Called when the candidate window is closed.
    virtual void CandidateWindowClosed(InputMethodManager* manager) = 0;
  };

  virtual ~InputMethodManager() {}

  // Adds an observer to receive notifications of input method related
  // changes as desribed in the Observer class above.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void AddCandidateWindowObserver(
      CandidateWindowObserver* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual void RemoveCandidateWindowObserver(
      CandidateWindowObserver* observer) = 0;

  // Sets the current browser status.
  virtual void SetState(State new_state) = 0;

  // Returns all input methods that are supported, including ones not active.
  // Caller has to delete the returned list. This function never returns NULL.
  // Note that input method extensions are NOT included in the result.
  virtual InputMethodDescriptors* GetSupportedInputMethods() const = 0;

  // Returns the list of input methods we can select (i.e. active) including
  // extension input methods. Caller has to delete the returned list.
  virtual InputMethodDescriptors* GetActiveInputMethods() const = 0;

  // Returns the number of active input methods including extension input
  // methods.
  virtual size_t GetNumActiveInputMethods() const = 0;

  // Changes the current input method to |input_method_id|. If |input_method_id|
  // is not active, switch to the first one in the active input method list.
  virtual void ChangeInputMethod(const std::string& input_method_id) = 0;

  // Enables keyboard layouts (e.g. US Qwerty, US Dvorak, French Azerty) that
  // are necessary for the |language_code| and then switches to |initial_layout|
  // if the string is not empty. For example, if |language_code| is "en-US", US
  // Qwerty, US International, US Extended, US Dvorak, and US Colemak layouts
  // would be enabled. Likewise, for Germany locale, US Qwerty which corresponds
  // to the hardware keyboard layout and several keyboard layouts for Germany
  // would be enabled.
  // This method is for setting up i18n keyboard layouts for the login screen.
  virtual void EnableLayouts(const std::string& language_code,
                             const std::string& initial_layout) = 0;

  // Activates the input method property specified by the |key|.
  virtual void ActivateInputMethodProperty(const std::string& key) = 0;

  // Updates the list of active input method IDs, and then starts or stops the
  // system input method framework as needed.
  virtual bool EnableInputMethods(
      const std::vector<std::string>& new_active_input_method_ids) = 0;

  // Updates a configuration of a system input method engine with |value|.
  // Returns true if the configuration is correctly set.
  virtual bool SetInputMethodConfig(const std::string& section,
                                    const std::string& config_name,
                                    const InputMethodConfigValue& value) = 0;

  // Adds an input method extension.
  virtual void AddInputMethodExtension(const std::string& id,
                                       const std::string& name,
                                       const std::vector<std::string>& layouts,
                                       const std::string& language) = 0;

  // Removes an input method extension.
  virtual void RemoveInputMethodExtension(const std::string& id) = 0;

  // Gets the descriptor of the input method which is currently selected.
  virtual InputMethodDescriptor GetCurrentInputMethod() const = 0;

  // Gets the list of input method properties. The list could be empty().
  virtual InputMethodPropertyList GetCurrentInputMethodProperties() const = 0;

  // Returns an X keyboard object which could be used to change the current XKB
  // layout, change the caps lock status, and set the auto repeat rate/interval.
  virtual XKeyboard* GetXKeyboard() = 0;

  // Returns an InputMethodUtil object.
  virtual InputMethodUtil* GetInputMethodUtil() = 0;

  // Enables all input method hotkeys such as Alt+Shift. Note that all hotkeys
  // are enabled on startup.
  virtual void EnableHotkeys() = 0;

  // Disables all input method hotkeys.
  virtual void DisableHotkeys() = 0;

  // Switches the current input method (or keyboard layout) to the next one.
  virtual bool SwitchToNextInputMethod() = 0;

  // Switches the current input method (or keyboard layout) to the previous one.
  virtual bool SwitchToPreviousInputMethod() = 0;

  // Switches to an input method (or keyboard layout) which is associated with
  // the |accelerator|.
  virtual bool SwitchInputMethod(const ui::Accelerator& accelerator) = 0;

  // Sets the global instance. Must be called before any calls to GetInstance().
  // We explicitly initialize and shut down the global object, rather than
  // making it a Singleton, to ensure clean startup and shutdown.
  static void Initialize();

  // Similar to Initialize(), but can inject an alternative
  // InputMethodManager such as MockInputMethodManager for testing.
  // The injected object will be owned by the internal pointer and deleted
  // by Shutdown().
  static void InitializeForTesting(InputMethodManager* mock_manager);

  // Destroys the global instance.
  static void Shutdown();

  // Gets the global instance. Initialize() or InitializeForTesting() must be
  // called first.
  static InputMethodManager* GetInstance();
};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_MANAGER_H_
