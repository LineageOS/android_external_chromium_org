// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_

#include <string>
#include <vector>

#include "build/build_config.h"

#include "base/stl_util.h"
#include "chrome/browser/password_manager/password_store.h"
#include "chrome/browser/password_manager/password_store_consumer.h"
#include "components/autofill/core/common/password_form.h"

namespace content {
class WebContents;
}  // namespace content

class PasswordManager;
class Profile;

// Per-password-form-{on-page, dialog} class responsible for interactions
// between a given form, the per-tab PasswordManager, and the PasswordStore.
class PasswordFormManager : public PasswordStoreConsumer {
 public:
  // profile contains the link to the PasswordStore and whether we're off
  //           the record
  // password_manager owns this object
  // form_on_page is the form that may be submitted and could need login data.
  // ssl_valid represents the security of the page containing observed_form,
  //           used to filter login results from database.
  PasswordFormManager(Profile* profile,
                      PasswordManager* password_manager,
                      content::WebContents* web_contents,
                      const autofill::PasswordForm& observed_form,
                      bool ssl_valid);
  virtual ~PasswordFormManager();

  enum ActionMatch {
    ACTION_MATCH_REQUIRED,
    ACTION_MATCH_NOT_REQUIRED
  };

  enum OtherPossibleUsernamesAction {
    ALLOW_OTHER_POSSIBLE_USERNAMES,
    IGNORE_OTHER_POSSIBLE_USERNAMES
  };

  // Compare basic data of observed_form_ with argument. Only check the action
  // URL when action match is required.
  bool DoesManage(const autofill::PasswordForm& form,
                  ActionMatch action_match) const;

  // Retrieves potential matching logins from the database.
  // |prompt_policy| indicates whether it's permissible to prompt the user to
  // authorize access to locked passwords. This argument is only used on
  // platforms that support prompting the user for access (such as Mac OS).
  void FetchMatchingLoginsFromPasswordStore(
      PasswordStore::AuthorizationPromptPolicy prompt_policy);

  // Retrieves potential matching logins from the database in order to display
  // them in the ManagePasswordsBubble. This is an async call, when the request
  // is completed, OnManagePasswordsBubbleRequestDone() is called. The fetched
  // passwords are not autofilled.
  void FetchMatchingLoginsFromPasswordStoreForBubble();

  // Simple state-check to verify whether this object as received a callback
  // from the PasswordStore and completed its matching phase. Note that the
  // callback in question occurs on the same (and only) main thread from which
  // instances of this class are ever used, but it is required since it is
  // conceivable that a user (or ui test) could attempt to submit a login
  // prompt before the callback has occured, which would InvokeLater a call to
  // PasswordManager::ProvisionallySave, which would interact with this object
  // before the db has had time to answer with matching password entries.
  // This is intended to be a one-time check; if the return value is false the
  // expectation is caller will give up. This clearly won't work if you put it
  // in a loop and wait for matching to complete; you're (supposed to be) on
  // the same thread!
  bool HasCompletedMatching();

  // Determines if the user opted to 'never remember' passwords for this form.
  bool IsBlacklisted();

  // Used by PasswordManager to determine whether or not to display
  // a SavePasswordBar when given the green light to save the PasswordForm
  // managed by this.
  bool IsNewLogin();

  // Returns true if the current pending credentials were found using
  // origin matching of the public suffix, instead of the signon realm of the
  // form.
  bool IsPendingCredentialsPublicSuffixMatch();

  // Checks if the form is a valid password form. Forms which lack either
  // login or password field are not considered valid.
  bool HasValidPasswordForm();

  // These functions are used to determine if this form has had it's password
  // auto generated by the browser.
  bool HasGeneratedPassword();
  void SetHasGeneratedPassword();

  // Determines if we need to autofill given the results of the query.
  void OnRequestDone(const std::vector<autofill::PasswordForm*>& result);

  // Creates the PasswordFormMap which is then given to the
  // ManagePasswordsBubble to update its logins.
  void OnManagePasswordsBubbleRequestDone(
      const std::vector<autofill::PasswordForm*>& result);

  // PasswordStoreConsumer implementation.
  virtual void OnPasswordStoreRequestDone(
      CancelableRequestProvider::Handle handle,
      const std::vector<autofill::PasswordForm*>& result) OVERRIDE;
  virtual void OnGetPasswordStoreResults(
      const std::vector<autofill::PasswordForm*>& results) OVERRIDE;

  // A user opted to 'never remember' passwords for this form.
  // Blacklist it so that from now on when it is seen we ignore it.
  // TODO: Make this private once we switch to the new UI.
  void PermanentlyBlacklist();

  // If the user has submitted observed_form_, provisionally hold on to
  // the submitted credentials until we are told by PasswordManager whether
  // or not the login was successful. |action| describes how we deal with
  // possible usernames. If |action| is ALLOW_OTHER_POSSIBLE_USERNAMES we will
  // treat a possible usernames match as a sign that our original heuristics
  // were wrong and that the user selected the correct username from the
  // Autofill UI.
  void ProvisionallySave(const autofill::PasswordForm& credentials,
                         OtherPossibleUsernamesAction action);

  // Handles save-as-new or update of the form managed by this manager.
  // Note the basic data of updated_credentials must match that of
  // observed_form_ (e.g DoesManage(pending_credentials_) == true).
  // TODO: Make this private once we switch to the new UI.
  void Save();

  // Call these if/when we know the form submission worked or failed.
  // These routines are used to update internal statistics ("ActionsTaken").
  void SubmitPassed();
  void SubmitFailed();

  // Returns the username associated with the credentials.
  const base::string16& associated_username() const {
    return pending_credentials_.username_value;
  }

  // Returns the pending credentials.
  const autofill::PasswordForm pending_credentials() const {
    return pending_credentials_;
  }

  // Returns the best matches.
  const autofill::PasswordFormMap best_matches() const {
    return best_matches_;
  }

  // Returns the realm URL for the form managed my this manager.
  const std::string& realm() const {
    return pending_credentials_.signon_realm;
  }

 private:
  friend class PasswordFormManagerTest;

  // ManagerAction - What does the manager do with this form? Either it
  // fills it, or it doesn't. If it doesn't fill it, that's either
  // because it has no match, or it is blacklisted, or it is disabled
  // via the AUTOCOMPLETE=off attribute. Note that if we don't have
  // an exact match, we still provide candidates that the user may
  // end up choosing.
  enum ManagerAction {
    kManagerActionNone = 0,
    kManagerActionAutofilled,
    kManagerActionBlacklisted,
    kManagerActionMax
  };

  // UserAction - What does the user do with this form? If he or she
  // does nothing (either by accepting what the password manager did, or
  // by simply (not typing anything at all), you get None. If there were
  // multiple choices and the user selects one other than the default,
  // you get Choose, if user selects an entry from matching against the Public
  // Suffix List you get ChoosePslMatch, and if the user types in a new value,
  // you get Override.
  enum UserAction {
    kUserActionNone = 0,
    kUserActionChoose,
    kUserActionChoosePslMatch,
    kUserActionOverride,
    kUserActionMax
  };

  // Result - What happens to the form?
  enum SubmitResult {
    kSubmitResultNotSubmitted = 0,
    kSubmitResultFailed,
    kSubmitResultPassed,
    kSubmitResultMax
  };

  // The maximum number of combinations of the three preceding enums.
  // This is used when recording the actions taken by the form in UMA.
  static const int kMaxNumActionsTaken = kManagerActionMax * kUserActionMax *
                                         kSubmitResultMax;

  // Helper for OnPasswordStoreRequestDone to determine whether or not
  // the given result form is worth scoring.
  bool IgnoreResult(const autofill::PasswordForm& form) const;

  // Helper for Save in the case that best_matches.size() == 0, meaning
  // we have no prior record of this form/username/password and the user
  // has opted to 'Save Password'. If |reset_preferred_login| is set,
  // the previously preferred login from |best_matches_| will be reset.
  void SaveAsNewLogin(bool reset_preferred_login);

  // Helper for OnPasswordStoreRequestDone to score an individual result
  // against the observed_form_.
  int ScoreResult(const autofill::PasswordForm& form) const;

  // Helper for Save in the case that best_matches.size() > 0, meaning
  // we have at least one match for this form/username/password. This
  // Updates the form managed by this object, as well as any matching forms
  // that now need to have preferred bit changed, since updated_credentials
  // is now implicitly 'preferred'.
  void UpdateLogin();

  // Check to see if |pending| corresponds to an account creation form. If we
  // think that it does, we label it as such and upload this state to the
  // Autofill server, so that we will trigger password generation in the future.
  void CheckForAccountCreationForm(const autofill::PasswordForm& pending,
                                   const autofill::PasswordForm& observed);

  // Update all login matches to reflect new preferred state - preferred flag
  // will be reset on all matched logins that different than the current
  // |pending_credentials_|.
  void UpdatePreferredLoginState(PasswordStore* password_store);

  // Returns true if |username| is one of the other possible usernames for a
  // password form in |best_matches_| and sets |pending_credentials_| to the
  // match which had this username.
  bool UpdatePendingCredentialsIfOtherPossibleUsername(
      const base::string16& username);

  // Converts the "ActionsTaken" fields into an int so they can be logged to
  // UMA.
  int GetActionsTaken();

  // Informs the renderer that the user has not blacklisted observed_form_ by
  // choosing "never save passwords for this site". This is used by the password
  // generation manager to deside whether to show the password generation icon.
  virtual void SendNotBlacklistedToRenderer();

  // Remove possible_usernames that may contains sensitive information and
  // duplicates.
  void SanitizePossibleUsernames(autofill::PasswordForm* form);

  // Set of PasswordForms from the DB that best match the form
  // being managed by this. Use a map instead of vector, because we most
  // frequently require lookups by username value in IsNewLogin.
  autofill::PasswordFormMap best_matches_;

  // Cleans up when best_matches_ goes out of scope.
  STLValueDeleter<autofill::PasswordFormMap> best_matches_deleter_;

  // The PasswordForm from the page or dialog managed by this.
  autofill::PasswordForm observed_form_;

  // The origin url path of observed_form_ tokenized, for convenience when
  // scoring.
  std::vector<std::string> form_path_tokens_;

  // Stores updated credentials when the form was submitted but success is
  // still unknown.
  autofill::PasswordForm pending_credentials_;

  // Whether pending_credentials_ stores a new login or is an update
  // to an existing one.
  bool is_new_login_;

  // Whether this form has an auto generated password.
  bool has_generated_password_;

  // Set if the user has selected one of the other possible usernames in
  // |pending_credentials_|.
  base::string16 selected_username_;

  // PasswordManager owning this.
  const PasswordManager* const password_manager_;

  // Convenience pointer to entry in best_matches_ that is marked
  // as preferred. This is only allowed to be null if there are no best matches
  // at all, since there will always be one preferred login when there are
  // multiple matches (when first saved, a login is marked preferred).
  const autofill::PasswordForm* preferred_match_;

  typedef enum {
    PRE_MATCHING_PHASE,      // Have not yet invoked a GetLogins query to find
                             // matching login information from password store.
    MATCHING_PHASE,          // We've made a GetLogins request, but
                             // haven't received or finished processing result.
    POST_MATCHING_PHASE      // We've queried the DB and processed matching
                             // login results.
  } PasswordFormManagerState;

  // State of matching process, used to verify that we don't call methods
  // assuming we've already processed the request for matching logins,
  // when we actually haven't.
  PasswordFormManagerState state_;

  // The profile from which we get the PasswordStore.
  Profile* profile_;

  // Web contents from which we get the RenderViewHost for sending messages to
  // the corresponding renderer.
  content::WebContents* web_contents_;

  // These three fields record the "ActionsTaken" by the browser and
  // the user with this form, and the result. They are combined and
  // recorded in UMA when the manager is destroyed.
  ManagerAction manager_action_;
  UserAction user_action_;
  SubmitResult submit_result_;

  // If the current request to GetLogins() came from the ManagePasswordsBubble,
  // passwords should not be autofilled but only given back to the bubble.
  bool manage_passwords_bubble_request_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormManager);
};
#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_
