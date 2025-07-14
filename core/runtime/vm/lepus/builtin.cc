// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/builtin.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/runtime/vm/lepus/array_api.h"
#include "core/runtime/vm/lepus/base_api.h"
#include "core/runtime/vm/lepus/date_api.h"
#include "core/runtime/vm/lepus/function_api.h"
#include "core/runtime/vm/lepus/json_api.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_date_api.h"
#include "core/runtime/vm/lepus/math_api.h"
#include "core/runtime/vm/lepus/regexp_api.h"
#include "core/runtime/vm/lepus/string_api.h"
#include "core/runtime/vm/lepus/table_api.h"

namespace lynx {
namespace lepus {

void RegisterCFunction(Context* context, const char* name, CFunction function) {
  VMContext::Cast(context)->SetGlobalData(name, lepus::Value(function));
}

void RegisterBuiltinFunction(Context* context, const char* name,
                             CFunction function) {
  VMContext::Cast(context)->SetBuiltinData(name, lepus::Value(function));
}

void RegisterBuiltinFunctionTable(Context* context, const char* name,
                                  BuiltinFunctionTable* function_table) {
  VMContext::Cast(context)->builtin()->Set(name, lepus::Value(function_table));
}

void RegisterFunctionTable(Context* context, const char* name,
                           fml::RefPtr<Dictionary> table) {
  VMContext::Cast(context)->global()->Set(name, lepus::Value(std::move(table)));
}

void RegisterFunctionTable(Context* context, const char* name,
                           BuiltinFunctionTable* function_table) {
  VMContext::Cast(context)->global()->Set(name, lepus::Value(function_table));
}

void RegisterTableFunction(Context* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunction function) {
  table->SetValue(name, function);
}

void RegisterBuiltin(Context* ctx) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, REGISTER_BUILD_IN);
  RegisterBaseAPI(ctx);
  RegisterStringAPI(ctx);
  RegisterMathAPI(ctx);
  RegisterDateAPI(ctx);
  RegisterJSONAPI(ctx);
  if (lynx::tasm::Config::IsHigherOrEqual(
          reinterpret_cast<VMContext*>(ctx)->GetSdkVersion(),
          FEATURE_CONTROL_VERSION_2)) {
    RegisterLepusDateAPI(ctx);
    RegisterFunctionAPI(ctx);
    RegisterTableAPI(ctx);
  }
}

}  // namespace lepus
}  // namespace lynx
