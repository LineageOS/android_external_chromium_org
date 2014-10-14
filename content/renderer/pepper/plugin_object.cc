// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/plugin_object.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "ppapi/c/dev/ppb_var_deprecated.h"
#include "ppapi/c/dev/ppp_class_deprecated.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/shared_impl/var_tracker.h"

using ppapi::PpapiGlobals;
using ppapi::ScopedPPVar;
using ppapi::ScopedPPVarArray;
using ppapi::StringVar;
using ppapi::Var;

namespace content {

namespace {

const char kInvalidValueException[] = "Error: Invalid value";

}  // namespace

// PluginObject ----------------------------------------------------------------

PluginObject::~PluginObject() {
  if (instance_) {
    ppp_class_->Deallocate(ppp_class_data_);
    instance_->RemovePluginObject(this);
  }
}

// static
gin::WrapperInfo PluginObject::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
PluginObject* PluginObject::FromV8Object(v8::Isolate* isolate,
                                         v8::Handle<v8::Object> v8_object) {
  PluginObject* plugin_object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &plugin_object)) {
    return plugin_object;
  }
  return NULL;
}

// static
PP_Var PluginObject::Create(PepperPluginInstanceImpl* instance,
                            const PPP_Class_Deprecated* ppp_class,
                            void* ppp_class_data) {
  PepperTryCatchVar try_catch(instance, NULL);
  gin::Handle<PluginObject> object =
      gin::CreateHandle(instance->GetIsolate(),
                        new PluginObject(instance, ppp_class, ppp_class_data));
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());
  return result.Release();
}

v8::Local<v8::Value> PluginObject::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  ScopedPPVar identifier_var(ScopedPPVar::PassRef(),
                             StringVar::StringToPPVar(identifier));
  return GetPropertyOrMethod(instance_->GetIsolate(), identifier_var.get());
}

bool PluginObject::SetNamedProperty(v8::Isolate* isolate,
                                    const std::string& identifier,
                                    v8::Local<v8::Value> value) {
  if (!instance_)
    return false;
  ScopedPPVar identifier_var(ScopedPPVar::PassRef(),
                             StringVar::StringToPPVar(identifier));
  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kAllowObjectVars,
                             isolate);

  bool has_property =
      ppp_class_->HasProperty(ppp_class_data_, identifier_var.get(),
                              try_catch.exception());
  if (try_catch.ThrowException())
    return false;

  if (!has_property)
    return false;

  ScopedPPVar var = try_catch.FromV8(value);
  if (try_catch.ThrowException())
    return false;

  ppp_class_->SetProperty(ppp_class_data_, identifier_var.get(), var.get(),
                          try_catch.exception());

  // If the plugin threw an exception, then throw a V8 version of it to
  // JavaScript. Either way, return true, because we successfully dispatched
  // the call to the plugin.
  try_catch.ThrowException();
  return true;
}

std::vector<std::string> PluginObject::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  if (!instance_)
    return result;

  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kAllowObjectVars,
                             isolate);

  PP_Var* name_vars;
  uint32_t count = 0;
  ppp_class_->GetAllPropertyNames(ppp_class_data_, &count, &name_vars,
                                  try_catch.exception());
  ScopedPPVarArray scoped_name_vars(
      ScopedPPVarArray::PassPPBMemoryAllocatedArray(), name_vars, count);

  if (try_catch.ThrowException())
    return result;

  for (uint32_t i = 0; i < count; ++i) {
    StringVar* string_var = StringVar::FromPPVar(name_vars[i]);
    if (string_var) {
      result.push_back(string_var->value());
    } else {
      try_catch.ThrowException(kInvalidValueException);
      result.clear();
      return result;
    }
  }

  return result;
}

void PluginObject::InstanceDeleted() {
  instance_ = NULL;
}

PluginObject::PluginObject(PepperPluginInstanceImpl* instance,
                           const PPP_Class_Deprecated* ppp_class,
                           void* ppp_class_data)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      ppp_class_(ppp_class),
      ppp_class_data_(ppp_class_data),
      weak_factory_(this) {
  instance_->AddPluginObject(this);
}

gin::ObjectTemplateBuilder PluginObject::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<PluginObject>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

v8::Local<v8::Value> PluginObject::GetPropertyOrMethod(v8::Isolate* isolate,
                                                       PP_Var identifier_var) {
  if (!instance_)
    return v8::Local<v8::Value>();

  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kAllowObjectVars,
                             isolate);
  bool has_property =
      ppp_class_->HasProperty(ppp_class_data_, identifier_var,
                              try_catch.exception());
  if (try_catch.ThrowException())
    return v8::Local<v8::Value>();

  if (has_property) {
    ScopedPPVar result_var(ScopedPPVar::PassRef(),
        ppp_class_->GetProperty(ppp_class_data_, identifier_var,
                                try_catch.exception()));
    if (try_catch.ThrowException())
      return v8::Local<v8::Value>();

    v8::Handle<v8::Value> result = try_catch.ToV8(result_var.get());
    if (try_catch.ThrowException())
      return v8::Local<v8::Value>();

    return result;
  }

  bool has_method = identifier_var.type == PP_VARTYPE_STRING &&
                    ppp_class_->HasMethod(ppp_class_data_, identifier_var,
                                          try_catch.exception());
  if (try_catch.ThrowException())
    return v8::Local<v8::Value>();

  if (has_method) {
    const std::string& identifier =
        StringVar::FromPPVar(identifier_var)->value();
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&PluginObject::Call,
                                                  weak_factory_.GetWeakPtr(),
                                                  identifier))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

void PluginObject::Call(const std::string& identifier,
                        gin::Arguments* args) {
  if (!instance_)
    return;

  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kAllowObjectVars,
                             args->isolate());
  ScopedPPVar identifier_var(ScopedPPVar::PassRef(),
                             StringVar::StringToPPVar(identifier));
  ScopedPPVarArray argument_vars(args->Length());

  for (uint32_t i = 0; i < argument_vars.size(); ++i) {
    v8::Handle<v8::Value> arg;
    if (!args->GetNext(&arg)) {
      NOTREACHED();
    }

    argument_vars.Set(i, try_catch.FromV8(arg));
    if (try_catch.ThrowException())
      return;
  }

  // For the OOP plugin case we need to grab a reference on the plugin module
  // object to ensure that it is not destroyed courtesy an incoming
  // ExecuteScript call which destroys the plugin module and in turn the
  // dispatcher.
  scoped_refptr<PluginModule> ref(instance_->module());

  ScopedPPVar result_var(ScopedPPVar::PassRef(),
      ppp_class_->Call(ppp_class_data_, identifier_var.get(),
                       argument_vars.size(), argument_vars.get(),
                       try_catch.exception()));
  if (try_catch.ThrowException())
    return;

  v8::Handle<v8::Value> result = try_catch.ToV8(result_var.get());
  if (try_catch.ThrowException())
    return;

  args->Return(result);
}

}  // namespace content
