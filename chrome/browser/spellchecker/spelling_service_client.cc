// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spelling_service_client.h"

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/spellcheck_result.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/google_api_keys.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "unicode/uloc.h"

// Use the public URL to the Spelling service on Chromium.
#ifndef SPELLING_SERVICE_URL
#define SPELLING_SERVICE_URL "https://www.googleapis.com/rpc"
#endif

SpellingServiceClient::SpellingServiceClient() : tag_(0) {
}

SpellingServiceClient::~SpellingServiceClient() {
}

bool SpellingServiceClient::RequestTextCheck(
    Profile* profile,
    int tag,
    ServiceType type,
    const string16& text,
    const TextCheckCompleteCallback& callback) {
  DCHECK(type == SUGGEST || type == SPELLCHECK);
  std::string locale = profile->GetPrefs()->GetString(
      prefs::kSpellCheckDictionary);
  char language[ULOC_LANG_CAPACITY] = ULOC_ENGLISH;
  const char* country = "USA";
  if (!locale.empty()) {
    // Create the parameters needed by Spelling API. Spelling API needs three
    // parameters: ISO language code, ISO3 country code, and text to be checked
    // by the service. On the other hand, Chrome uses an ISO locale ID and it
    // may not include a country ID, e.g. "fr", "de", etc. To create the input
    // parameters, we convert the UI locale to a full locale ID, and  convert
    // the full locale ID to an ISO language code and and ISO3 country code.
    // Also, we convert the given text to a JSON string, i.e. quote all its
    // non-ASCII characters.
    UErrorCode error = U_ZERO_ERROR;
    char id[ULOC_LANG_CAPACITY + ULOC_SCRIPT_CAPACITY + ULOC_COUNTRY_CAPACITY];
    uloc_addLikelySubtags(locale.c_str(), id, arraysize(id), &error);

    error = U_ZERO_ERROR;
    uloc_getLanguage(id, language, arraysize(language), &error);
    country = uloc_getISO3Country(id);
  }
  if (!IsAvailable(profile, type))
    return false;

  // Format the JSON request to be sent to the Spelling service.
  std::string encoded_text;
  base::JsonDoubleQuote(text, false, &encoded_text);

  static const char kSpellingRequest[] =
      "{"
      "\"method\":\"spelling.check\","
      "\"apiVersion\":\"v%d\","
      "\"params\":{"
      "\"text\":\"%s\","
      "\"language\":\"%s\","
      "\"originCountry\":\"%s\","
      "\"key\":\"%s\""
      "}"
      "}";
  std::string api_key = google_apis::GetAPIKey();
  std::string request = base::StringPrintf(
      kSpellingRequest,
      type,
      encoded_text.c_str(),
      language,
      country,
      api_key.c_str());

  static const char kSpellingServiceURL[] = SPELLING_SERVICE_URL;
  GURL url = GURL(kSpellingServiceURL);
  fetcher_.reset(CreateURLFetcher(url));
  fetcher_->SetRequestContext(profile->GetRequestContext());
  fetcher_->SetUploadData("application/json", request);
  fetcher_->SetLoadFlags(
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES);
  fetcher_->Start();
  tag_ = tag;
  text_ = text;
  callback_ = callback;
  return true;
}

bool SpellingServiceClient::IsAvailable(Profile* profile, ServiceType type) {
  const PrefService* pref = profile->GetPrefs();
  if (!pref->GetBoolean(prefs::kEnableSpellCheck) ||
      !pref->GetBoolean(prefs::kSpellCheckUseSpellingService))
    return false;

  // If the locale for spelling has not been set, the user has not decided to
  // use spellcheck so we don't do anything remote (suggest or spelling).
  std::string locale = pref->GetString(prefs::kSpellCheckDictionary);
  if (locale.empty())
    return false;

  // If we do not have grammar check enabled, then we are only available for
  // SUGGEST.
  // If we do not have asynchronous spelling turned on, then we are only
  // available for SUGGEST. The SPELLCHECK option depends on asynchronous
  // spelling.
  const CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kUseSpellingService) ||
      command_line->HasSwitch(switches::kForceSyncSpellCheck))
    return type == SUGGEST;

  // Finally, if all options are available, we only enable only SUGGEST
  // if SPELLCHECK is not available for our language because SPELLCHECK results
  // are a superset of SUGGEST results.
  // TODO(rlp): Only available for English right now. Fix this line to include
  // all languages SPELLCHECK covers.
  bool language_available = !locale.compare(0, 2, "en");
  if (language_available) {
    // Either SUGGEST or SPELLCHECK are allowed.
    return true;
  } else {
    // Only SUGGEST is allowed.
    return type == SUGGEST;
  }
}

void SpellingServiceClient::OnURLFetchComplete(
    const net::URLFetcher* source) {
  scoped_ptr<net::URLFetcher> clean_up_fetcher(fetcher_.release());
  bool success = false;
  std::vector<SpellCheckResult> results;
  if (source->GetResponseCode() / 100 == 2) {
    std::string data;
    source->GetResponseAsString(&data);
    success = ParseResponse(data, &results);
  }
  callback_.Run(tag_, success, text_, results);
}

net::URLFetcher* SpellingServiceClient::CreateURLFetcher(const GURL& url) {
  return net::URLFetcher::Create(url, net::URLFetcher::POST, this);
}

bool SpellingServiceClient::ParseResponse(
    const std::string& data,
    std::vector<SpellCheckResult>* results) {
  // When this JSON-RPC call finishes successfully, the Spelling service returns
  // an JSON object listed below.
  // * result - an envelope object representing the result from the APIARY
  //   server, which is the JSON-API front-end for the Spelling service. This
  //   object consists of the following variable:
  //   - spellingCheckResponse (SpellingCheckResponse).
  // * SpellingCheckResponse - an object representing the result from the
  //   Spelling service. This object consists of the following variable:
  //   - misspellings (optional array of Misspelling)
  // * Misspelling - an object representing a misspelling region and its
  //   suggestions. This object consists of the following variables:
  //   - charStart (number) - the beginning of the misspelled region;
  //   - charLength (number) - the length of the misspelled region;
  //   - suggestions (array of string) - the suggestions for the misspelling
  //     text, and;
  //   - canAutoCorrect (optional boolean) - whether we can use the first
  //     suggestion for auto-correction.
  // For example, the Spelling service returns the following JSON when we send a
  // spelling request for "duck goes quisk" as of 16 August, 2011.
  // {
  //   "result": {
  //     "spellingCheckResponse": {
  //       "misspellings": [{
  //           "charStart": 10,
  //           "charLength": 5,
  //           "suggestions": [{ "suggestion": "quack" }],
  //           "canAutoCorrect": false
  //       }]
  //     }
  //   }
  // }
  scoped_ptr<DictionaryValue> value(
      static_cast<DictionaryValue*>(
          base::JSONReader::Read(data, base::JSON_ALLOW_TRAILING_COMMAS)));
  if (!value.get() || !value->IsType(base::Value::TYPE_DICTIONARY))
    return false;

  // Retrieve the array of Misspelling objects. When the input text does not
  // have misspelled words, it returns an empty JSON. (In this case, its HTTP
  // status is 200.) We just return true for this case.
  ListValue* misspellings = NULL;
  const char kMisspellings[] = "result.spellingCheckResponse.misspellings";
  if (!value->GetList(kMisspellings, &misspellings))
    return true;

  for (size_t i = 0; i < misspellings->GetSize(); ++i) {
    // Retrieve the i-th misspelling region and put it to the given vector. When
    // the Spelling service sends two or more suggestions, we read only the
    // first one because SpellCheckResult can store only one suggestion.
    DictionaryValue* misspelling = NULL;
    if (!misspellings->GetDictionary(i, &misspelling))
      return false;

    int start = 0;
    int length = 0;
    ListValue* suggestions = NULL;
    if (!misspelling->GetInteger("charStart", &start) ||
        !misspelling->GetInteger("charLength", &length) ||
        !misspelling->GetList("suggestions", &suggestions)) {
      return false;
    }

    DictionaryValue* suggestion = NULL;
    string16 replacement;
    if (!suggestions->GetDictionary(0, &suggestion) ||
        !suggestion->GetString("suggestion", &replacement)) {
      return false;
    }
    SpellCheckResult result(
        SpellCheckResult::SPELLING, start, length, replacement);
    results->push_back(result);
  }
  return true;
}
