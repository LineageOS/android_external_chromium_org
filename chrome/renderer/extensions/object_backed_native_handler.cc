// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(dcarney): Remove this when UnsafePersistent is removed.
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR

#include "chrome/renderer/extensions/object_backed_native_handler.h"

#include "base/logging.h"
#include "base/memory/linked_ptr.h"
#include "chrome/renderer/extensions/chrome_v8_context.h"
#include "chrome/renderer/extensions/console.h"
#include "chrome/renderer/extensions/module_system.h"
#include "v8/include/v8.h"

namespace extensions {

namespace {
// Key for the base::Bound routed function.
const char* kHandlerFunction = "handler_function";
}  // namespace

ObjectBackedNativeHandler::ObjectBackedNativeHandler(ChromeV8Context* context)
    : router_data_(context->v8_context()->GetIsolate()),
      context_(context),
      object_template_(
          v8::ObjectTemplate::New(context->v8_context()->GetIsolate())) {}

ObjectBackedNativeHandler::~ObjectBackedNativeHandler() {
  Invalidate();
}

v8::Handle<v8::Object> ObjectBackedNativeHandler::NewInstance() {
  return object_template_.NewHandle(v8::Isolate::GetCurrent())->NewInstance();
}

// static
void ObjectBackedNativeHandler::Router(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope handle_scope(args.GetIsolate());
  v8::Handle<v8::Object> data = args.Data().As<v8::Object>();

  v8::Handle<v8::Value> handler_function_value =
      data->Get(v8::String::NewFromUtf8(args.GetIsolate(), kHandlerFunction));
  // See comment in header file for why we do this.
  if (handler_function_value.IsEmpty() ||
      handler_function_value->IsUndefined()) {
    console::Error(args.GetIsolate()->GetCallingContext(),
                   "Extension view no longer exists");
    return;
  }
  DCHECK(handler_function_value->IsExternal());
  static_cast<HandlerFunction*>(
      handler_function_value.As<v8::External>()->Value())->Run(args);
}

void ObjectBackedNativeHandler::RouteFunction(
    const std::string& name,
    const HandlerFunction& handler_function) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context_->v8_context());

  v8::Persistent<v8::Object> data(isolate, v8::Object::New(isolate));
  v8::Local<v8::Object> local_data = v8::Local<v8::Object>::New(isolate, data);
  local_data->Set(
      v8::String::NewFromUtf8(isolate, kHandlerFunction),
      v8::External::New(isolate, new HandlerFunction(handler_function)));
  v8::Handle<v8::FunctionTemplate> function_template =
      v8::FunctionTemplate::New(isolate, Router, local_data);
  object_template_.NewHandle(isolate)
      ->Set(isolate, name.c_str(), function_template);
  router_data_.Append(local_data);
}

v8::Isolate* ObjectBackedNativeHandler::GetIsolate() const {
  return context_->isolate();
}

void ObjectBackedNativeHandler::Invalidate() {
  if (!is_valid())
    return;
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context_->v8_context());

  for (size_t i = 0; i < router_data_.Size(); i++) {
    v8::Handle<v8::Object> data = router_data_.Get(i);
    v8::Handle<v8::Value> handler_function_value =
        data->Get(v8::String::NewFromUtf8(isolate, kHandlerFunction));
    CHECK(!handler_function_value.IsEmpty());
    delete static_cast<HandlerFunction*>(
        handler_function_value.As<v8::External>()->Value());
    data->Delete(v8::String::NewFromUtf8(isolate, kHandlerFunction));
  }
  router_data_.Clear();
  object_template_.reset();
  context_ = NULL;
  NativeHandler::Invalidate();
}

}   // namespace extensions
