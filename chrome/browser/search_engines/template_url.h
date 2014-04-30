// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_H_
#define CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_H_

#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/autocomplete/autocomplete_input.h"
#include "chrome/browser/search_engines/template_url_id.h"
#include "ui/gfx/size.h"
#include "url/gurl.h"
#include "url/url_parse.h"

class Profile;
class SearchTermsData;
class TemplateURL;


// TemplateURLRef -------------------------------------------------------------

// A TemplateURLRef represents a single URL within the larger TemplateURL class
// (which represents an entire "search engine", see below).  If
// SupportsReplacement() is true, this URL has placeholders in it, for which
// callers can substitute values to get a "real" URL using ReplaceSearchTerms().
//
// TemplateURLRefs always have a non-NULL |owner_| TemplateURL, which they
// access in order to get at important data like the underlying URL string or
// the associated Profile.
class TemplateURLRef {
 public:
  // Magic numbers to pass to ReplaceSearchTerms() for the |accepted_suggestion|
  // parameter.  Most callers aren't using Suggest capabilities and should just
  // pass NO_SUGGESTIONS_AVAILABLE.
  // NOTE: Because positive values are meaningful, make sure these are negative!
  enum AcceptedSuggestion {
    NO_SUGGESTION_CHOSEN = -1,
    NO_SUGGESTIONS_AVAILABLE = -2,
  };

  // Which kind of URL within our owner we are.  This allows us to get at the
  // correct string field. Use |INDEXED| to indicate that the numerical
  // |index_in_owner_| should be used instead.
  enum Type {
    SEARCH,
    SUGGEST,
    INSTANT,
    IMAGE,
    NEW_TAB,
    INDEXED
  };

  // Type to store <content_type, post_data> pair for POST URLs.
  // The |content_type|(first part of the pair) is the content-type of
  // the |post_data|(second part of the pair) which is encoded in
  // "multipart/form-data" format, it also contains the MIME boundary used in
  // the |post_data|. See http://tools.ietf.org/html/rfc2046 for the details.
  typedef std::pair<std::string, std::string> PostContent;

  // This struct encapsulates arguments passed to
  // TemplateURLRef::ReplaceSearchTerms methods.  By default, only search_terms
  // is required and is passed in the constructor.
  struct SearchTermsArgs {
    explicit SearchTermsArgs(const base::string16& search_terms);
    ~SearchTermsArgs();

    // The search terms (query).
    base::string16 search_terms;

    // The original (input) query.
    base::string16 original_query;

    // The optional assisted query stats, aka AQS, used for logging purposes.
    // This string contains impressions of all autocomplete matches shown
    // at the query submission time.  For privacy reasons, we require the
    // search provider to support HTTPS protocol in order to receive the AQS
    // param.
    // For more details, see http://goto.google.com/binary-clients-logging .
    std::string assisted_query_stats;

    // TODO: Remove along with "aq" CGI param.
    int accepted_suggestion;

    // The 0-based position of the cursor within the query string at the time
    // the request was issued.  Set to base::string16::npos if not used.
    size_t cursor_position;

    // The start-edge margin of the omnibox in pixels, used in extended Instant
    // to align the preview contents with the omnibox.
    int omnibox_start_margin;

    // The URL of the current webpage to be used for experimental zero-prefix
    // suggestions.
    std::string current_page_url;

    // Which omnibox the user used to type the prefix.
    AutocompleteInput::PageClassification page_classification;

    // True for searches issued with the bookmark bar pref set to shown.
    bool bookmark_bar_pinned;

    // Additional query params provided by the suggest server.
    std::string suggest_query_params;

    // If set, ReplaceSearchTerms() will automatically append any extra query
    // params specified via the --extra-search-query-params command-line
    // argument.  Generally, this should be set when dealing with the search or
    // instant TemplateURLRefs of the default search engine and the caller cares
    // about the query portion of the URL.  Since neither TemplateURLRef nor
    // indeed TemplateURL know whether a TemplateURL is the default search
    // engine, callers instead must set this manually.
    bool append_extra_query_params;

    // The raw content of an image thumbnail that will be used as a query for
    // search-by-image frontend.
    std::string image_thumbnail_content;

    // When searching for an image, the URL of the original image. Callers
    // should leave this empty for images specified via data: URLs.
    GURL image_url;

    // When searching for an image, the original size of the image.
    gfx::Size image_original_size;

    // If set, ReplaceSearchTerms() will append a param to the TemplateURLRef to
    // update the search results page incrementally even if that is otherwise
    // disabled by google.com preferences. See comments on
    // chrome::ForceInstantResultsParam().
    bool force_instant_results;

    // True if the search was made using the app list search box. Otherwise, the
    // search was made using the omnibox.
    bool from_app_list;
  };

  TemplateURLRef(TemplateURL* owner, Type type);
  TemplateURLRef(TemplateURL* owner, size_t index_in_owner);
  ~TemplateURLRef();

  // Returns the raw URL. None of the parameters will have been replaced.
  std::string GetURL() const;

  // Returns the raw string of the post params. Please see comments in
  // prepopulated_engines_schema.json for the format.
  std::string GetPostParamsString() const;

  // Returns true if this URL supports search term replacement.
  bool SupportsReplacement() const;

  // Like SupportsReplacement but usable on threads other than the UI thread.
  bool SupportsReplacementUsingTermsData(
      const SearchTermsData& search_terms_data) const;

  // Returns a string that is the result of replacing the search terms in
  // the url with the specified arguments.  We use our owner's input encoding.
  //
  // If this TemplateURLRef does not support replacement (SupportsReplacement
  // returns false), an empty string is returned.
  // If this TemplateURLRef uses POST, and |post_content| is not NULL, the
  // |post_params_| will be replaced, encoded in "multipart/form-data" format
  // and stored into |post_content|.
  std::string ReplaceSearchTerms(
      const SearchTermsArgs& search_terms_args,
      PostContent* post_content) const;
  // TODO(jnd): remove the following ReplaceSearchTerms definition which does
  // not have |post_content| parameter once all reference callers pass
  // |post_content| parameter.
  std::string ReplaceSearchTerms(
      const SearchTermsArgs& search_terms_args) const {
    return ReplaceSearchTerms(search_terms_args, NULL);
  }

  // Just like ReplaceSearchTerms except that it takes SearchTermsData to supply
  // the data for some search terms. Most of the time ReplaceSearchTerms should
  // be called.
  std::string ReplaceSearchTermsUsingTermsData(
      const SearchTermsArgs& search_terms_args,
      const SearchTermsData& search_terms_data,
      PostContent* post_content) const;

  // Returns true if the TemplateURLRef is valid. An invalid TemplateURLRef is
  // one that contains unknown terms, or invalid characters.
  bool IsValid() const;

  // Like IsValid but usable on threads other than the UI thread.
  bool IsValidUsingTermsData(const SearchTermsData& search_terms_data) const;

  // Returns a string representation of this TemplateURLRef suitable for
  // display. The display format is the same as the format used by Firefox.
  base::string16 DisplayURL() const;

  // Converts a string as returned by DisplayURL back into a string as
  // understood by TemplateURLRef.
  static std::string DisplayURLToURLRef(const base::string16& display_url);

  // If this TemplateURLRef is valid and contains one search term, this returns
  // the host/path of the URL, otherwise this returns an empty string.
  const std::string& GetHost() const;
  const std::string& GetPath() const;

  // If this TemplateURLRef is valid and contains one search term, this returns
  // the key of the search term, otherwise this returns an empty string.
  const std::string& GetSearchTermKey() const;

  // Converts the specified term in our owner's encoding to a base::string16.
  base::string16 SearchTermToString16(const std::string& term) const;

  // Returns true if this TemplateURLRef has a replacement term of
  // {google:baseURL} or {google:baseSuggestURL}.
  bool HasGoogleBaseURLs() const;

  // Use the pattern referred to by this TemplateURLRef to match the provided
  // |url| and extract |search_terms| from it. Returns true if the pattern
  // matches, even if |search_terms| is empty. In this case
  // |search_term_component|, if not NULL, indicates whether the search terms
  // were found in the query or the ref parameters; and |search_terms_position|,
  // if not NULL, contains the position of the search terms in the query or the
  // ref parameters. Returns false and an empty |search_terms| if the pattern
  // does not match.
  bool ExtractSearchTermsFromURL(
      const GURL& url,
      base::string16* search_terms,
      const SearchTermsData& search_terms_data,
      url_parse::Parsed::ComponentType* search_term_component,
      url_parse::Component* search_terms_position) const;

  // Whether the URL uses POST (as opposed to GET).
  bool UsesPOSTMethodUsingTermsData(
      const SearchTermsData* search_terms_data) const;
  bool UsesPOSTMethod() const {
    return UsesPOSTMethodUsingTermsData(NULL);
  }

 private:
  friend class TemplateURL;
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, SetPrepopulatedAndParse);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseParameterKnown);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseParameterUnknown);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseURLEmpty);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseURLNoTemplateEnd);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseURLNoKnownParameters);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseURLTwoParameters);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ParseURLNestedParameter);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, URLRefTestImageURLWithPOST);
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ReflectsBookmarkBarPinned);

  // Enumeration of the known types.
  enum ReplacementType {
    ENCODING,
    GOOGLE_ASSISTED_QUERY_STATS,
    GOOGLE_BASE_URL,
    GOOGLE_BASE_SUGGEST_URL,
    GOOGLE_BOOKMARK_BAR_PINNED,
    GOOGLE_CURRENT_PAGE_URL,
    GOOGLE_CURSOR_POSITION,
    GOOGLE_IMAGE_ORIGINAL_HEIGHT,
    GOOGLE_IMAGE_ORIGINAL_WIDTH,
    GOOGLE_IMAGE_SEARCH_SOURCE,
    GOOGLE_IMAGE_THUMBNAIL,
    GOOGLE_IMAGE_URL,
    GOOGLE_FORCE_INSTANT_RESULTS,
    GOOGLE_INSTANT_EXTENDED_ENABLED,
    GOOGLE_NTP_IS_THEMED,
    GOOGLE_OMNIBOX_START_MARGIN,
    GOOGLE_ORIGINAL_QUERY_FOR_SUGGESTION,
    GOOGLE_PAGE_CLASSIFICATION,
    GOOGLE_RLZ,
    GOOGLE_SEARCH_CLIENT,
    GOOGLE_SEARCH_FIELDTRIAL_GROUP,
    GOOGLE_SUGGEST_CLIENT,
    GOOGLE_SUGGEST_REQUEST_ID,
    GOOGLE_UNESCAPED_SEARCH_TERMS,
    LANGUAGE,
    SEARCH_TERMS,
  };

  // Used to identify an element of the raw url that can be replaced.
  struct Replacement {
    Replacement(ReplacementType type, size_t index)
        : type(type), index(index), is_post_param(false) {}
    ReplacementType type;
    size_t index;
    // Indicates the location in where the replacement is replaced. If
    // |is_post_param| is false, |index| indicates the byte position in
    // |parsed_url_|. Otherwise, |index| is the index of |post_params_|.
    bool is_post_param;
  };

  // The list of elements to replace.
  typedef std::vector<struct Replacement> Replacements;
  // Type to store <key, value> pairs for POST URLs.
  typedef std::pair<std::string, std::string> PostParam;
  typedef std::vector<PostParam> PostParams;

  // TemplateURLRef internally caches values to make replacement quick. This
  // method invalidates any cached values.
  void InvalidateCachedValues() const;

  // Parses the parameter in url at the specified offset. start/end specify the
  // range of the parameter in the url, including the braces. If the parameter
  // is valid, url is updated to reflect the appropriate parameter. If
  // the parameter is one of the known parameters an element is added to
  // replacements indicating the type and range of the element. The original
  // parameter is erased from the url.
  //
  // If the parameter is not a known parameter, false is returned. If this is a
  // prepopulated URL, the parameter is erased, otherwise it is left alone.
  bool ParseParameter(size_t start,
                      size_t end,
                      std::string* url,
                      Replacements* replacements) const;

  // Parses the specified url, replacing parameters as necessary. If
  // successful, valid is set to true, and the parsed url is returned. For all
  // known parameters that are encountered an entry is added to replacements.
  // If there is an error parsing the url, valid is set to false, and an empty
  // string is returned.  If the URL has the POST parameters, they will be
  // parsed into |post_params| which will be further replaced with real search
  // terms data and encoded in "multipart/form-data" format to generate the
  // POST data.
  std::string ParseURL(const std::string& url,
                       Replacements* replacements,
                       PostParams* post_params,
                       bool* valid) const;

  // If the url has not yet been parsed, ParseURL is invoked.
  // NOTE: While this is const, it modifies parsed_, valid_, parsed_url_ and
  // search_offset_.
  void ParseIfNecessary() const;

  // Like ParseIfNecessary but usable on threads other than the UI thread.
  void ParseIfNecessaryUsingTermsData(
      const SearchTermsData& search_terms_data) const;

  // Extracts the query key and host from the url.
  void ParseHostAndSearchTermKey(
      const SearchTermsData& search_terms_data) const;

  // Encode post parameters in "multipart/form-data" format and store it
  // inside |post_content|. Returns false if errors are encountered during
  // encoding. This method is called each time ReplaceSearchTerms gets called.
  bool EncodeFormData(const PostParams& post_params,
                      PostContent* post_content) const;

  // Handles a replacement by using real term data. If the replacement
  // belongs to a PostParam, the PostParam will be replaced by the term data.
  // Otherwise, the term data will be inserted at the place that the
  // replacement points to.
  void HandleReplacement(const std::string& name,
                         const std::string& value,
                         const Replacement& replacement,
                         std::string* url) const;

  // Replaces all replacements in |parsed_url_| with their actual values and
  // returns the result.  This is the main functionality of
  // ReplaceSearchTermsUsingTermsData().
  std::string HandleReplacements(
      const SearchTermsArgs& search_terms_args,
      const SearchTermsData& search_terms_data,
      PostContent* post_content) const;

  // The TemplateURL that contains us.  This should outlive us.
  TemplateURL* const owner_;

  // What kind of URL we are.
  const Type type_;

  // If |type_| is |INDEXED|, this |index_in_owner_| is used instead to refer to
  // a url within our owner.
  const size_t index_in_owner_;

  // Whether the URL has been parsed.
  mutable bool parsed_;

  // Whether the url was successfully parsed.
  mutable bool valid_;

  // The parsed URL. All terms have been stripped out of this with
  // replacements_ giving the index of the terms to replace.
  mutable std::string parsed_url_;

  // Do we support search term replacement?
  mutable bool supports_replacements_;

  // The replaceable parts of url (parsed_url_). These are ordered by index
  // into the string, and may be empty.
  mutable Replacements replacements_;

  // Host, path, key and location of the search term. These are only set if the
  // url contains one search term.
  mutable std::string host_;
  mutable std::string path_;
  mutable std::string search_term_key_;
  mutable url_parse::Parsed::ComponentType search_term_key_location_;

  mutable PostParams post_params_;

  // Whether the contained URL is a pre-populated URL.
  bool prepopulated_;

  // Whether search terms are shown in the omnibox on search results pages.
  // This is kept as a member so it can be overridden by tests.
  bool showing_search_terms_;

  DISALLOW_COPY_AND_ASSIGN(TemplateURLRef);
};


// TemplateURLData ------------------------------------------------------------

// The data for the TemplateURL.  Separating this into its own class allows most
// users to do SSA-style usage of TemplateURL: construct a TemplateURLData with
// whatever fields are desired, then create an immutable TemplateURL from it.
struct TemplateURLData {
  TemplateURLData();
  ~TemplateURLData();

  // A short description of the template. This is the name we show to the user
  // in various places that use TemplateURLs. For example, the location bar
  // shows this when the user selects a substituting match.
  base::string16 short_name;

  // The shortcut for this TemplateURL.  |keyword| must be non-empty.
  void SetKeyword(const base::string16& keyword);
  const base::string16& keyword() const { return keyword_; }

  // The raw URL for the TemplateURL, which may not be valid as-is (e.g. because
  // it requires substitutions first).  This must be non-empty.
  void SetURL(const std::string& url);
  const std::string& url() const { return url_; }

  // Optional additional raw URLs.
  std::string suggestions_url;
  std::string instant_url;
  std::string image_url;
  std::string new_tab_url;

  // The following post_params are comma-separated lists used to specify the
  // post parameters for the corresponding URL.
  std::string search_url_post_params;
  std::string suggestions_url_post_params;
  std::string instant_url_post_params;
  std::string image_url_post_params;

  // Optional favicon for the TemplateURL.
  GURL favicon_url;

  // URL to the OSD file this came from. May be empty.
  GURL originating_url;

  // Whether this TemplateURL is shown in the default list of search providers.
  // This is just a property and does not indicate whether the TemplateURL has a
  // TemplateURLRef that supports replacement. Use
  // TemplateURL::ShowInDefaultList() to test both.
  bool show_in_default_list;

  // Whether it's safe for auto-modification code (the autogenerator and the
  // code that imports data from other browsers) to replace the TemplateURL.
  // This should be set to false for any TemplateURL the user edits, or any
  // TemplateURL that the user clearly manually edited in the past, like a
  // bookmark keyword from another browser.
  bool safe_for_autoreplace;

  // The list of supported encodings for the search terms. This may be empty,
  // which indicates the terms should be encoded with UTF-8.
  std::vector<std::string> input_encodings;

  // Unique identifier of this TemplateURL. The unique ID is set by the
  // TemplateURLService when the TemplateURL is added to it.
  TemplateURLID id;

  // Date this TemplateURL was created.
  //
  // NOTE: this may be 0, which indicates the TemplateURL was created before we
  // started tracking creation time.
  base::Time date_created;

  // The last time this TemplateURL was modified by a user, since creation.
  //
  // NOTE: Like date_created above, this may be 0.
  base::Time last_modified;

  // True if this TemplateURL was automatically created by the administrator via
  // group policy.
  bool created_by_policy;

  // Number of times this TemplateURL has been explicitly used to load a URL.
  // We don't increment this for uses as the "default search engine" since
  // that's not really "explicit" usage and incrementing would result in pinning
  // the user's default search engine(s) to the top of the list of searches on
  // the New Tab page, de-emphasizing the omnibox as "where you go to search".
  int usage_count;

  // If this TemplateURL comes from prepopulated data the prepopulate_id is > 0.
  int prepopulate_id;

  // The primary unique identifier for Sync. This set on all TemplateURLs
  // regardless of whether they have been associated with Sync.
  std::string sync_guid;

  // A list of URL patterns that can be used, in addition to |url_|, to extract
  // search terms from a URL.
  std::vector<std::string> alternate_urls;

  // A parameter that, if present in the query or ref parameters of a search_url
  // or instant_url, causes Chrome to replace the URL with the search term.
  std::string search_terms_replacement_key;

 private:
  // Private so we can enforce using the setters and thus enforce that these
  // fields are never empty.
  base::string16 keyword_;
  std::string url_;
};


// AssociatedExtensionInfo ----------------------------------------------------

// An AssociatedExtensionInfo represents information about the extension that
// added the search engine using the Override Settings API.
struct AssociatedExtensionInfo {
  std::string extension_id;

  // Whether the search engine is supposed to be default.
  bool wants_to_be_default_engine;

  // Used to resolve conflicts when there are multiple extensions specifying the
  // default search engine. The most recently-installed wins.
  base::Time install_time;
};


// TemplateURL ----------------------------------------------------------------

// A TemplateURL represents a single "search engine", defined primarily as a
// subset of the Open Search Description Document
// (http://www.opensearch.org/Specifications/OpenSearch) plus some extensions.
// One TemplateURL contains several TemplateURLRefs, which correspond to various
// different capabilities (e.g. doing searches or getting suggestions), as well
// as a TemplateURLData containing other details like the name, keyword, etc.
//
// TemplateURLs are intended to be read-only for most users; the only public
// non-const method is the Profile getter, which returns a non-const Profile*.
// The TemplateURLService, which handles storing and manipulating TemplateURLs,
// is made a friend so that it can be the exception to this pattern.
class TemplateURL {
 public:
  enum Type {
    // Regular search engine.
    NORMAL,
    // Installed by extension through Override Settings API.
    NORMAL_CONTROLLED_BY_EXTENSION,
    // The keyword associated with an extension that uses the Omnibox API.
    OMNIBOX_API_EXTENSION,
  };
  // |profile| may be NULL.  This will affect the results of e.g. calling
  // ReplaceSearchTerms() on the member TemplateURLRefs.
  TemplateURL(Profile* profile, const TemplateURLData& data);
  ~TemplateURL();

  // Generates a favicon URL from the specified url.
  static GURL GenerateFaviconURL(const GURL& url);

  Profile* profile() { return profile_; }
  const TemplateURLData& data() const { return data_; }

  const base::string16& short_name() const { return data_.short_name; }
  // An accessor for the short_name, but adjusted so it can be appropriately
  // displayed even if it is LTR and the UI is RTL.
  base::string16 AdjustedShortNameForLocaleDirection() const;

  const base::string16& keyword() const { return data_.keyword(); }

  const std::string& url() const { return data_.url(); }
  const std::string& suggestions_url() const { return data_.suggestions_url; }
  const std::string& instant_url() const { return data_.instant_url; }
  const std::string& image_url() const { return data_.image_url; }
  const std::string& new_tab_url() const { return data_.new_tab_url; }
  const std::string& search_url_post_params() const {
    return data_.search_url_post_params;
  }
  const std::string& suggestions_url_post_params() const {
    return data_.suggestions_url_post_params;
  }
  const std::string& instant_url_post_params() const {
    return data_.instant_url_post_params;
  }
  const std::string& image_url_post_params() const {
    return data_.image_url_post_params;
  }
  const std::vector<std::string>& alternate_urls() const {
    return data_.alternate_urls;
  }
  const GURL& favicon_url() const { return data_.favicon_url; }

  const GURL& originating_url() const { return data_.originating_url; }

  bool show_in_default_list() const { return data_.show_in_default_list; }
  // Returns true if show_in_default_list() is true and this TemplateURL has a
  // TemplateURLRef that supports replacement.
  bool ShowInDefaultList() const;

  bool safe_for_autoreplace() const { return data_.safe_for_autoreplace; }

  const std::vector<std::string>& input_encodings() const {
    return data_.input_encodings;
  }

  TemplateURLID id() const { return data_.id; }

  base::Time date_created() const { return data_.date_created; }
  base::Time last_modified() const { return data_.last_modified; }

  bool created_by_policy() const { return data_.created_by_policy; }

  int usage_count() const { return data_.usage_count; }

  int prepopulate_id() const { return data_.prepopulate_id; }

  const std::string& sync_guid() const { return data_.sync_guid; }

  // TODO(beaudoin): Rename this when renaming HasSearchTermsReplacementKey().
  const std::string& search_terms_replacement_key() const {
    return data_.search_terms_replacement_key;
  }

  const TemplateURLRef& url_ref() const { return url_ref_; }
  const TemplateURLRef& suggestions_url_ref() const {
    return suggestions_url_ref_;
  }
  const TemplateURLRef& instant_url_ref() const { return instant_url_ref_; }
  const TemplateURLRef& image_url_ref() const { return image_url_ref_; }
  const TemplateURLRef& new_tab_url_ref() const { return new_tab_url_ref_; }

  // Returns true if |url| supports replacement.
  bool SupportsReplacement() const;

  // Like SupportsReplacement but usable on threads other than the UI thread.
  bool SupportsReplacementUsingTermsData(
      const SearchTermsData& search_terms_data) const;

  // Returns true if this TemplateURL uses Google base URLs and has a keyword
  // of "google.TLD".  We use this to decide whether we can automatically
  // update the keyword to reflect the current Google base URL TLD.
  bool IsGoogleSearchURLWithReplaceableKeyword() const;

  // Returns true if the keywords match or if
  // IsGoogleSearchURLWithReplaceableKeyword() is true for both |this| and
  // |other|.
  bool HasSameKeywordAs(const TemplateURLData& other) const;

  Type GetType() const;

  // Returns the id of the extension that added this search engine. Only call
  // this for TemplateURLs of type NORMAL_CONTROLLED_BY_EXTENSION or
  // OMNIBOX_API_EXTENSION.
  std::string GetExtensionId() const;

  // Returns the total number of URLs comprised in this template, including
  // search and alternate URLs.
  size_t URLCount() const;

  // Gets the search URL at the given index. The alternate URLs, if any, are
  // numbered starting at 0, and the primary search URL follows. This is used
  // to decode the search term given a search URL (see
  // ExtractSearchTermsFromURL()).
  const std::string& GetURL(size_t index) const;

  // Use the alternate URLs and the search URL to match the provided |url|
  // and extract |search_terms| from it. Returns false and an empty
  // |search_terms| if no search terms can be matched. The order in which the
  // alternate URLs are listed dictates their priority, the URL at index 0 is
  // treated as the highest priority and the primary search URL is treated as
  // the lowest priority (see GetURL()).  For example, if a TemplateURL has
  // alternate URL "http://foo/#q={searchTerms}" and search URL
  // "http://foo/?q={searchTerms}", and the URL to be decoded is
  // "http://foo/?q=a#q=b", the alternate URL will match first and the decoded
  // search term will be "b".
  bool ExtractSearchTermsFromURL(const GURL& url, base::string16* search_terms);

  // Like ExtractSearchTermsFromURL but usable on threads other than the UI
  // thread.
  bool ExtractSearchTermsFromURLUsingTermsData(
      const GURL& url,
      base::string16* search_terms,
      const SearchTermsData& search_terms_data);

  // Returns true if non-empty search terms could be extracted from |url| using
  // ExtractSearchTermsFromURL(). In other words, this returns whether |url|
  // could be the result of performing a search with |this|.
  bool IsSearchURL(const GURL& url);

  // Like IsSearchURL but usable on threads other than the UI thread.
  bool IsSearchURLUsingTermsData(
      const GURL& url,
      const SearchTermsData& search_terms_data);

  // Returns true if the specified |url| contains the search terms replacement
  // key in either the query or the ref. This method does not verify anything
  // else about the URL. In particular, it does not check that the domain
  // matches that of this TemplateURL.
  // TODO(beaudoin): Rename this to reflect that it really checks for an
  // InstantExtended capable URL.
  bool HasSearchTermsReplacementKey(const GURL& url) const;

  // Given a |url| corresponding to this TemplateURL, identifies the search
  // terms and replaces them with the ones in |search_terms_args|, leaving the
  // other parameters untouched. If the replacement fails, returns false and
  // leaves |result| untouched. This is used by mobile ports to perform query
  // refinement.
  bool ReplaceSearchTermsInURL(
      const GURL& url,
      const TemplateURLRef::SearchTermsArgs& search_terms_args,
      GURL* result);

  // Encodes the search terms from |search_terms_args| so that we know the
  // |input_encoding|. Returns the |encoded_terms| and the
  // |encoded_original_query|. |encoded_terms| may be escaped as path or query
  // depending on |is_in_query|; |encoded_original_query| is always escaped as
  // query.
  void EncodeSearchTerms(
      const TemplateURLRef::SearchTermsArgs& search_terms_args,
      bool is_in_query,
      std::string* input_encoding,
      base::string16* encoded_terms,
      base::string16* encoded_original_query) const;

 private:
  friend class TemplateURLService;
  FRIEND_TEST_ALL_PREFIXES(TemplateURLTest, ReflectsBookmarkBarPinned);

  void CopyFrom(const TemplateURL& other);

  void SetURL(const std::string& url);
  void SetPrepopulateId(int id);

  // Resets the keyword if IsGoogleSearchURLWithReplaceableKeyword() or |force|.
  // The |force| parameter is useful when the existing keyword is known to be
  // a placeholder.  The resulting keyword is generated using
  // TemplateURLService::GenerateSearchURL() and
  // TemplateURLService::GenerateKeyword().
  void ResetKeywordIfNecessary(bool force);

  // Uses the alternate URLs and the search URL to match the provided |url|
  // and extract |search_terms| from it as well as the |search_terms_component|
  // (either REF or QUERY) and |search_terms_component| at which the
  // |search_terms| are found in |url|. See also ExtractSearchTermsFromURL().
  bool FindSearchTermsInURL(
      const GURL& url,
      const SearchTermsData& search_terms_data,
      base::string16* search_terms,
      url_parse::Parsed::ComponentType* search_terms_component,
      url_parse::Component* search_terms_position);

  Profile* profile_;
  TemplateURLData data_;
  TemplateURLRef url_ref_;
  TemplateURLRef suggestions_url_ref_;
  TemplateURLRef instant_url_ref_;
  TemplateURLRef image_url_ref_;
  TemplateURLRef new_tab_url_ref_;
  scoped_ptr<AssociatedExtensionInfo> extension_info_;

  // TODO(sky): Add date last parsed OSD file.

  DISALLOW_COPY_AND_ASSIGN(TemplateURL);
};

#endif  // CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_H_
