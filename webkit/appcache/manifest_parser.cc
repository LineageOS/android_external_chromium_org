// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This is a port of ManifestParser.cc from WebKit/WebCore/loader/appcache.

/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "webkit/appcache/manifest_parser.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "googleurl/src/gurl.h"

namespace appcache {

enum Mode {
  kExplicit,
  kFallback,
  kOnlineWhitelist,
  kUnknown,
};

bool ParseManifest(const GURL& manifest_url, const char* data, int length,
                   Manifest& manifest) {
  static const std::wstring kSignature(L"CACHE MANIFEST");

  DCHECK(manifest.explicit_urls.empty());
  DCHECK(manifest.fallback_namespaces.empty());
  DCHECK(manifest.online_whitelist_namespaces.empty());
  DCHECK(!manifest.online_whitelist_all);

  Mode mode = kExplicit;

  std::wstring data_string;
  // TODO(jennb): cannot do UTF8ToWide(data, length, &data_string);
  // until UTF8ToWide uses 0xFFFD Unicode replacement character.
  CodepageToWide(std::string(data, length), "UTF-8",
                 OnStringUtilConversionError::SUBSTITUTE, &data_string);
  const wchar_t* p = data_string.c_str();
  const wchar_t* end = p + data_string.length();

  // Look for the magic signature: "^\xFEFF?CACHE MANIFEST[ \t]?"
  // Example: "CACHE MANIFEST #comment" is a valid signature.
  // Example: "CACHE MANIFEST;V2" is not.

  // When the input data starts with a UTF-8 Byte-Order-Mark
  // (0xEF, 0xBB, 0xBF), the UTF8ToWide() function converts it to a
  // Unicode BOM (U+FEFF). Skip a converted Unicode BOM if it exists.
  int bom_offset = 0;
  if (!data_string.empty() && data_string[0] == 0xFEFF) {
    bom_offset = 1;
    ++p;
  }

  if (p >= end ||
      data_string.compare(bom_offset, kSignature.length(), kSignature)) {
    return false;
  }

  p += kSignature.length();     // Skip past "CACHE MANIFEST"

  // Character after "CACHE MANIFEST" must be whitespace.
  if (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
    return false;

  // Skip to the end of the line.
  while (p < end && *p != '\r' && *p != '\n')
    ++p;

  while (1) {
    // Skip whitespace
    while (p < end && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
      ++p;

    if (p == end)
      break;

    const wchar_t* line_start = p;

    // Find the end of the line
    while (p < end && *p != '\r' && *p != '\n')
      ++p;

    // Check if we have a comment
    if (*line_start == '#')
      continue;

    // Get rid of trailing whitespace
    const wchar_t* tmp = p - 1;
    while (tmp > line_start && (*tmp == ' ' || *tmp == '\t'))
      --tmp;

    std::wstring line(line_start, tmp - line_start + 1);

    if (line == L"CACHE:") {
      mode = kExplicit;
    } else if (line == L"FALLBACK:") {
      mode = kFallback;
    } else if (line == L"NETWORK:") {
      mode = kOnlineWhitelist;
    } else if (*(line.end() - 1) == ':') {
      mode = kUnknown;
    } else if (mode == kUnknown) {
      continue;
    } else if (line == L"*" && mode == kOnlineWhitelist) {
      manifest.online_whitelist_all = true;
      continue;
    } else if (mode == kExplicit || mode == kOnlineWhitelist) {
      const wchar_t *line_p = line.c_str();
      const wchar_t *line_end = line_p + line.length();

      // Look for whitespace separating the URL from subsequent ignored tokens.
      while (line_p < line_end && *line_p != '\t' && *p != ' ')
        ++line_p;

      string16 url16;
      WideToUTF16(line.c_str(), line_p - line.c_str(), &url16);
      GURL url = manifest_url.Resolve(url16);
      if (!url.is_valid())
        continue;
      if (url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        url = url.ReplaceComponents(replacements);
      }

      // Scheme component must be the same as the manifest URL's.
      if (url.scheme() != manifest_url.scheme()) {
        continue;
      }

      if (mode == kExplicit) {
        manifest.explicit_urls.insert(url.spec());
      } else {
        manifest.online_whitelist_namespaces.push_back(url);
      }
    } else if (mode == kFallback) {
      const wchar_t* line_p = line.c_str();
      const wchar_t* line_end = line_p + line.length();

      // Look for whitespace separating the two URLs
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      if (line_p == line_end) {
        // There was no whitespace separating the URLs.
        continue;
      }

      string16 namespace_url16;
      WideToUTF16(line.c_str(), line_p - line.c_str(), &namespace_url16);
      GURL namespace_url = manifest_url.Resolve(namespace_url16);
      if (!namespace_url.is_valid())
        continue;
      if (namespace_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        namespace_url = namespace_url.ReplaceComponents(replacements);
      }

      // Fallback namespace URL must have the same scheme, host and port
      // as the manifest's URL.
      if (manifest_url.GetOrigin() != namespace_url.GetOrigin()) {
        continue;
      }

      // Skip whitespace separating fallback namespace from URL.
      while (line_p < line_end && (*line_p == '\t' || *line_p == ' '))
        ++line_p;

      // Look for whitespace separating the URL from subsequent ignored tokens.
      const wchar_t* fallback_start = line_p;
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      string16 fallback_url16;
      WideToUTF16(fallback_start, line_p - fallback_start, &fallback_url16);
      GURL fallback_url = manifest_url.Resolve(fallback_url16);
      if (!fallback_url.is_valid())
        continue;
      if (fallback_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        fallback_url = fallback_url.ReplaceComponents(replacements);
      }

      // Fallback entry URL must have the same scheme, host and port
      // as the manifest's URL.
      if (manifest_url.GetOrigin() != fallback_url.GetOrigin()) {
        continue;
      }

      // Store regardless of duplicate namespace URL. Only first match
      // will ever be used.
      manifest.fallback_namespaces.push_back(
          FallbackNamespace(namespace_url, fallback_url));
    } else {
      NOTREACHED();
    }
  }

  return true;
}

}  // namespace appcache
