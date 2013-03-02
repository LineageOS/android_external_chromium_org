// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_MANIFEST_H_
#define CHROME_COMMON_EXTENSIONS_MANIFEST_H_

#include <map>
#include <string>
#include <set>

#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "base/values.h"

namespace extensions {
struct InstallWarning;

// Wraps the DictionaryValue form of extension's manifest. Enforces access to
// properties of the manifest using ManifestFeatureProvider.
class Manifest {
 public:
  // What an extension was loaded from.
  // NOTE: These values are stored as integers in the preferences and used
  // in histograms so don't remove or reorder existing items.  Just append
  // to the end.
  enum Location {
    INVALID_LOCATION,
    INTERNAL,           // A crx file from the internal Extensions directory.
    EXTERNAL_PREF,      // A crx file from an external directory (via prefs).
    EXTERNAL_REGISTRY,  // A crx file from an external directory (via eg the
                        // registry on Windows).
    UNPACKED,           // From loading an unpacked extension from the
                        // extensions settings page.
    COMPONENT,          // An integral component of Chrome itself, which
                        // happens to be implemented as an extension. We don't
                        // show these in the management UI.
    EXTERNAL_PREF_DOWNLOAD,    // A crx file from an external directory (via
                               // prefs), installed from an update URL.
    EXTERNAL_POLICY_DOWNLOAD,  // A crx file from an external directory (via
                               // admin policies), installed from an update URL.
    COMMAND_LINE,       // --load-extension.

    NUM_LOCATIONS
  };

  // Do not change the order of entries or remove entries in this list
  // as this is used in UMA_HISTOGRAM_ENUMERATIONs about extensions.
  enum Type {
    TYPE_UNKNOWN = 0,
    TYPE_EXTENSION,
    TYPE_THEME,
    TYPE_USER_SCRIPT,
    TYPE_HOSTED_APP,
    // This is marked legacy because platform apps are preferred. For
    // backwards compatibility, we can't remove support for packaged apps
    TYPE_LEGACY_PACKAGED_APP,
    TYPE_PLATFORM_APP
  };

  // Given two install sources, return the one which should take priority
  // over the other. If an extension is installed from two sources A and B,
  // its install source should be set to GetHigherPriorityLocation(A, B).
  static Location GetHigherPriorityLocation(Location loc1, Location loc2);

  // Whether the |location| is external or not.
  static inline bool IsExternalLocation(Location location) {
    return location == EXTERNAL_PREF ||
           location == EXTERNAL_REGISTRY ||
           location == EXTERNAL_PREF_DOWNLOAD ||
           location == EXTERNAL_POLICY_DOWNLOAD;
  }

  // Whether the |location| is unpacked (no CRX) or not.
  static inline bool IsUnpackedLocation(Location location) {
    return location == UNPACKED || location == COMMAND_LINE;
  }

  // Whether extensions with |location| are auto-updatable or not.
  static inline bool IsAutoUpdateableLocation(Location location) {
    // Only internal and external extensions can be autoupdated.
    return location == INTERNAL ||
           IsExternalLocation(location);
  }

  // Unpacked extensions start off with file access since they are a developer
  // feature.
  static inline bool ShouldAlwaysAllowFileAccess(Location location) {
    return IsUnpackedLocation(location);
  }

  Manifest(Location location, scoped_ptr<DictionaryValue> value);
  virtual ~Manifest();

  const std::string& extension_id() const { return extension_id_; }
  void set_extension_id(const std::string& id) { extension_id_ = id; }

  Location location() const { return location_; }

  // Returns false and |error| will be non-empty if the manifest is malformed.
  // |warnings| will be populated if there are keys in the manifest that cannot
  // be specified by the extension type.
  bool ValidateManifest(std::string* error,
                        std::vector<InstallWarning>* warnings) const;

  // The version of this extension's manifest. We increase the manifest
  // version when making breaking changes to the extension system. If the
  // manifest contains no explicit manifest version, this returns the current
  // system default.
  int GetManifestVersion() const;

  // Returns the manifest type.
  Type type() const { return type_; }

  bool is_theme() const { return type_ == TYPE_THEME; }
  bool is_platform_app() const { return type_ == TYPE_PLATFORM_APP; }
  bool is_hosted_app() const { return type_ == TYPE_HOSTED_APP; }
  bool is_legacy_packaged_app() const {
    return type_ == TYPE_LEGACY_PACKAGED_APP;
  }
  bool is_extension() const { return type_ == TYPE_EXTENSION; }

  // These access the wrapped manifest value, returning false when the property
  // does not exist or if the manifest type can't access it.
  bool HasKey(const std::string& key) const;
  bool HasPath(const std::string& path) const;
  bool Get(const std::string& path, const base::Value** out_value) const;
  bool GetBoolean(const std::string& path, bool* out_value) const;
  bool GetInteger(const std::string& path, int* out_value) const;
  bool GetString(const std::string& path, std::string* out_value) const;
  bool GetString(const std::string& path, string16* out_value) const;
  bool GetDictionary(const std::string& path,
                     const base::DictionaryValue** out_value) const;
  bool GetList(const std::string& path,
               const base::ListValue** out_value) const;

  // Returns a new Manifest equal to this one, passing ownership to
  // the caller.
  Manifest* DeepCopy() const;

  // Returns true if this equals the |other| manifest.
  bool Equals(const Manifest* other) const;

  // Gets the underlying DictionaryValue representing the manifest.
  // Note: only use this when you KNOW you don't need the validation.
  const base::DictionaryValue* value() const { return value_.get(); }

 private:
  // Returns true if the extension can specify the given |path|.
  bool CanAccessPath(const std::string& path) const;
  bool CanAccessKey(const std::string& key) const;

  // A persistent, globally unique ID. An extension's ID is used in things
  // like directory structures and URLs, and is expected to not change across
  // versions. It is generated as a SHA-256 hash of the extension's public
  // key, or as a hash of the path in the case of unpacked extensions.
  std::string extension_id_;

  // The location the extension was loaded from.
  Location location_;

  // The underlying dictionary representation of the manifest.
  scoped_ptr<base::DictionaryValue> value_;

  Type type_;

  DISALLOW_COPY_AND_ASSIGN(Manifest);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_MANIFEST_H_
