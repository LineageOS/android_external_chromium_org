// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: jasonh@google.com (Jason Hsueh)
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.
// It provides reflection support for generated enums, and is included in
// generated .pb.h files and should have minimal dependencies. The methods are
// implemented in generated_message_reflection.cc.

#ifndef GOOGLE_PROTOBUF_GENERATED_ENUM_REFLECTION_H__
#define GOOGLE_PROTOBUF_GENERATED_ENUM_REFLECTION_H__

#include <string>

namespace google {
namespace protobuf {
  class EnumDescriptor;
}  // namespace protobuf

namespace protobuf {

// Returns the EnumDescriptor for enum type E, which must be a
// proto-declared enum type.  Code generated by the protocol compiler
// will include specializations of this template for each enum type declared.
template <typename E>
const EnumDescriptor* GetEnumDescriptor();

namespace internal {

// Helper for EnumType_Parse functions: try to parse the string 'name' as an
// enum name of the given type, returning true and filling in value on success,
// or returning false and leaving value unchanged on failure.
LIBPROTOBUF_EXPORT bool ParseNamedEnum(const EnumDescriptor* descriptor,
                    const string& name,
                    int* value);

template<typename EnumType>
bool ParseNamedEnum(const EnumDescriptor* descriptor,
                    const string& name,
                    EnumType* value) {
  int tmp;
  if (!ParseNamedEnum(descriptor, name, &tmp)) return false;
  *value = static_cast<EnumType>(tmp);
  return true;
}

// Just a wrapper around printing the name of a value. The main point of this
// function is not to be inlined, so that you can do this without including
// descriptor.h.
LIBPROTOBUF_EXPORT const string& NameOfEnum(const EnumDescriptor* descriptor, int value);

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_GENERATED_ENUM_REFLECTION_H__
