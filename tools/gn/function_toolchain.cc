// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/err.h"
#include "tools/gn/functions.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/scope.h"
#include "tools/gn/settings.h"
#include "tools/gn/toolchain.h"

namespace functions {

namespace {

// This is jsut a unique value to take the address of to use as the key for
// the toolchain property on a scope.
const int kToolchainPropertyKey = 0;

// Reads the given string from the scope (if present) and puts the result into
// dest. If the value is not a string, sets the error and returns false.
bool ReadString(Scope& scope, const char* var, std::string* dest, Err* err) {
  const Value* v = scope.GetValue(var, true);
  if (!v)
    return true;  // Not present is fine.

  if (!v->VerifyTypeIs(Value::STRING, err))
    return false;
  *dest = v->string_value();
  return true;
}

}  // namespace

// toolchain -------------------------------------------------------------------

const char kToolchain[] = "toolchain";
const char kToolchain_Help[] =
    "TODO(brettw) write this.";

Value RunToolchain(Scope* scope,
                   const FunctionCallNode* function,
                   const std::vector<Value>& args,
                   BlockNode* block,
                   Err* err) {
  if (!EnsureNotProcessingImport(function, scope, err) ||
      !EnsureNotProcessingBuildConfig(function, scope, err))
    return Value();

  // Note that we don't want to use MakeLabelForScope since that will include
  // the toolchain name in the label, and toolchain labels don't themselves
  // have toolchain names.
  const SourceDir& input_dir = scope->GetSourceDir();
  Label label(input_dir, args[0].string_value(), SourceDir(), std::string());
  if (g_scheduler->verbose_logging())
    g_scheduler->Log("Generating toolchain", label.GetUserVisibleName(false));

  // This object will actually be copied into the one owned by the toolchain
  // manager, but that has to be done in the lock.
  Toolchain toolchain(label);

  Scope block_scope(scope);
  block_scope.SetProperty(&kToolchainPropertyKey, &toolchain);
  block->ExecuteBlockInScope(&block_scope, err);
  block_scope.SetProperty(&kToolchainPropertyKey, NULL);
  if (err->has_error())
    return Value();
  if (!block_scope.CheckForUnusedVars(err))
    return Value();

  const BuildSettings* build_settings = scope->settings()->build_settings();
  {
    // Save the toolchain definition in the toolchain manager and mark the
    // corresponding item in the dependency tree resolved so that targets
    // that depend on this toolchain know it's ready.
    base::AutoLock lock(build_settings->item_tree().lock());
    build_settings->toolchain_manager().SetToolchainDefinitionLocked(
        toolchain, function->GetRange(), err);
    build_settings->item_tree().MarkItemDefinedLocked(build_settings, label,
                                                      err);
  }
  return Value();
}

// tool ------------------------------------------------------------------------

const char kTool[] = "tool";
const char kTool_Help[] =
    "TODO(brettw) write this.";

Value RunTool(Scope* scope,
              const FunctionCallNode* function,
              const std::vector<Value>& args,
              BlockNode* block,
              Err* err) {
  // Find the toolchain definition we're executing inside of. The toolchain
  // function will set a property pointing to it that we'll pick up.
  Toolchain* toolchain = reinterpret_cast<Toolchain*>(
      scope->GetProperty(&kToolchainPropertyKey, NULL));
  if (!toolchain) {
    *err = Err(function->function(), "tool() called outside of toolchain().",
        "The tool() function can only be used inside a toolchain() "
        "definition.");
    return Value();
  }

  if (!EnsureSingleStringArg(function, args, err))
    return Value();
  const std::string& tool_name = args[0].string_value();
  Toolchain::ToolType tool_type = Toolchain::ToolNameToType(tool_name);
  if (tool_type == Toolchain::TYPE_NONE) {
    *err = Err(args[0], "Unknown tool type");
    return Value();
  }

  // Run the tool block.
  Scope block_scope(scope);
  block->ExecuteBlockInScope(&block_scope, err);
  if (err->has_error())
    return Value();

  // Extract the stuff we need.
  Toolchain::Tool t;
  if (!ReadString(block_scope, "command", &t.command, err) ||
      !ReadString(block_scope, "depfile", &t.depfile, err) ||
      !ReadString(block_scope, "deps", &t.deps, err) ||
      !ReadString(block_scope, "description", &t.description, err) ||
      !ReadString(block_scope, "pool", &t.pool, err) ||
      !ReadString(block_scope, "restat", &t.restat, err) ||
      !ReadString(block_scope, "rspfile", &t.rspfile, err) ||
      !ReadString(block_scope, "rspfile_content", &t.rspfile_content, err))
    return Value();

  // Make sure there weren't any vars set in this tool that were unused.
  if (!block_scope.CheckForUnusedVars(err))
    return Value();

  toolchain->SetTool(tool_type, t);
  return Value();
}

// toolchain_args --------------------------------------------------------------

extern const char kToolchainArgs[] = "toolchain_args";
extern const char kToolchainArgs_Help[] =
    "toolchain_args: Set build arguments for toolchain build setup.\n"
    "\n"
    "  When you specify a target using an alternate toolchain, the master\n"
    "  build configuration file is re-interpreted in the context of that\n"
    "  toolchain. This function allows you to control the arguments passed\n"
    "  into this alternate invocation of the build.\n"
    "\n"
    "  Any default system arguments or arguments passed in on the command-\n"
    "  line will also be passed to the alternate invocation unless explicitly\n"
    "  overriddey by toolchain_args.\n"
    "\n"
    "  The toolchain_args will be ignored when the toolchain being defined\n"
    "  is the default. In this case, it's expected you want the default\n"
    "  argument values.\n"
    "\n"
    "  See also \"gn help buildargs\" for an overview of these arguments.\n"
    "\n"
    "Example:\n"
    "  toolchain(\"my_weird_toolchain\") {\n"
    "    ...\n"
    "    toolchain_args() {\n"
    "      # Override the system values for a generic Posix system.\n"
    "      is_win = false\n"
    "      is_posix = true\n"
    "\n"
    "      # Pass this new value for specific setup for my toolchain.\n"
    "      is_my_weird_system = true\n"
    "    }\n"
    "  }\n";

Value RunToolchainArgs(Scope* scope,
                       const FunctionCallNode* function,
                       const std::vector<Value>& args,
                       BlockNode* block,
                       Err* err) {
  // Find the toolchain definition we're executing inside of. The toolchain
  // function will set a property pointing to it that we'll pick up.
  Toolchain* toolchain = reinterpret_cast<Toolchain*>(
      scope->GetProperty(&kToolchainPropertyKey, NULL));
  if (!toolchain) {
    *err = Err(function->function(),
               "toolchain_args() called outside of toolchain().",
               "The toolchain_args() function can only be used inside a "
               "toolchain() definition.");
    return Value();
  }

  if (!args.empty()) {
    *err = Err(function->function(), "This function takes no arguments.");
    return Value();
  }

  // This function makes a new scope with various variable sets on it, which
  // we then save on the toolchain to use when re-invoking the build.
  Scope block_scope(scope);
  block->ExecuteBlockInScope(&block_scope, err);
  if (err->has_error())
    return Value();

  Scope::KeyValueMap values;
  block_scope.GetCurrentScopeValues(&values);
  toolchain->args() = values;

  return Value();
}

}  // namespace functions
