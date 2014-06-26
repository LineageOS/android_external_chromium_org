// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <set>
#include <string>

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

// Verifies a logging stanza.
// |keyValuePairs| lists the keys that must have specified values, and |keys|
// lists the keys that must be present, but may have arbitrary values.
// There must be no other keys.
bool VerifyStanza(
    const std::map<std::string, std::string>& key_value_pairs,
    const std::set<std::string> keys,
    const buzz::XmlElement* elem,
    std::string* error);

}  // namespace remoting
