// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_ENGINES_SEARCH_TERMS_DATA_H_
#define COMPONENTS_SEARCH_ENGINES_SEARCH_TERMS_DATA_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/strings/string16.h"

// All data needed by TemplateURLRef::ReplaceSearchTerms which typically may
// only be accessed on the UI thread.
class SearchTermsData {
 public:
  SearchTermsData();
  virtual ~SearchTermsData();

  // Returns the value to use for replacements of type GOOGLE_BASE_URL.  This
  // implementation simply returns the default value.
  virtual std::string GoogleBaseURLValue() const;

  // Returns the value for the GOOGLE_BASE_SUGGEST_URL term.  This
  // implementation simply returns the default value.
  std::string GoogleBaseSuggestURLValue() const;

  // Returns the locale used by the application.  This implementation returns
  // "en" and thus should be overridden where the result is actually meaningful.
  virtual std::string GetApplicationLocale() const;

  // Returns the value for the Chrome Omnibox rlz.  This implementation returns
  // the empty string.
  virtual base::string16 GetRlzParameterValue(bool from_app_list) const;

  // The optional client parameter passed with Google search requests.  This
  // implementation returns the empty string.
  virtual std::string GetSearchClient() const;

  // The suggest client parameter ("client") passed with Google suggest
  // requests.  See GetSuggestRequestIdentifier() for more details.
  // This implementation returns the empty string.
  virtual std::string GetSuggestClient() const;

  // The suggest request identifier parameter ("gs_ri") passed with Google
  // suggest requests.   Along with suggestclient (See GetSuggestClient()),
  // this parameter controls what suggestion results are returned.
  // This implementation returns the empty string.
  virtual std::string GetSuggestRequestIdentifier() const;

  // Returns a string indicating whether a non-default theme is active,
  // suitable for adding as a query string param to the homepage.  This only
  // applies if Instant Extended is enabled.  Returns an empty string otherwise.
  // Determining this requires accessing the Profile, so this can only ever be
  // non-empty for UIThreadSearchTermsData.
  virtual std::string NTPIsThemedParam() const;

  // Returns the value to use for replacements of type
  // GOOGLE_IMAGE_SEARCH_SOURCE.
  virtual std::string GoogleImageSearchSource() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SearchTermsData);
};

#endif  // COMPONENTS_SEARCH_ENGINES_SEARCH_TERMS_DATA_H_
