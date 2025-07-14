// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/harmony/native_module_harmony.h"

#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_napi_env_holder.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace harmony {

struct CallbackData {
  CallbackData(const std::shared_ptr<piper::LynxModuleCallback>& c,
               const std::shared_ptr<piper::LynxNativeModule::Delegate>& d)
      : callback(c), delegate(d) {}
  std::shared_ptr<piper::LynxModuleCallback> callback;
  std::weak_ptr<piper::LynxNativeModule::Delegate> delegate;
#if ENABLE_TRACE_PERFETTO
  std::string module_name;
  std::string method_name;
  uint64_t flow_id;
  std::string first_arg;
#endif
};

NativeModuleHarmony::NativeModuleHarmony(
    const std::shared_ptr<PlatformModuleManager>& manager, napi_env env,
    const std::string& name, bool sendable,
    const std::vector<std::string>& methods)
    : platform_manager_(manager),
      main_env_(env),
      sendable_(sendable),
      module_name_(name) {
  for (const auto& m : methods) {
    methods_.emplace(m, piper::NativeModuleMethod(m, 0));
  }
}

napi_value NativeModuleHarmony::InvokeMethod(
    napi_env env, const lepus::Value& lepus_argv,
    const std::string& method_name, const std::shared_ptr<Delegate>& delegate,
    const piper::CallbackMap& callbacks, uint64_t flow_id,
    const std::string& first_arg) const {
  auto manager = platform_manager_.lock();
  if (!manager) {
    return nullptr;
  }

  base::NapiHandleScope scope(env);
  napi_value platform_get_module_func =
      manager->JSGetModuleFunc(env, sendable_);
  napi_value receiver = manager->JSModuleManager(env, sendable_);
  if (platform_get_module_func == nullptr || receiver == nullptr) {
    return nullptr;
  }
  napi_value module_name;
  napi_create_string_latin1(env, module_name_.c_str(), NAPI_AUTO_LENGTH,
                            &module_name);
  napi_value get_module_args[1] = {module_name};
  napi_value platform_module = nullptr;
  napi_call_function(env, receiver, platform_get_module_func, 1,
                     get_module_args, &platform_module);
  if (platform_module == nullptr) {
    return nullptr;
  }
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, LEPUS_VALUE_TO_NAPI_VALUE);
  size_t size = static_cast<size_t>(lepus_argv.GetLength());
  std::vector<napi_value> argv(size);
  for (size_t i = 0; i < size; i++) {
    auto value = lepus_argv.Array()->get(i);
    // If the current parameter is an int64 value, it might originally have
    // been a JS callback, with this int64 value being its ID. The
    // corresponding callback instance can be found in the callback map using
    // the current index.
    if (value.IsInt64() && callbacks.find(i) != callbacks.end()) {
      auto callback_data = new CallbackData(callbacks.at(i), delegate);
#if ENABLE_TRACE_PERFETTO
      callback_data->module_name = module_name_;
      callback_data->method_name = method_name;
      callback_data->flow_id = flow_id;
      callback_data->first_arg = first_arg;
#endif
      napi_create_function(env, "callback", 9, NativeModuleHarmony::Callback,
                           callback_data, &argv[i]);
    } else {
      argv[i] = base::NapiConvertHelper::CreateNapiValue(env, value);
    }
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);

  napi_value target_method;
  napi_get_named_property(env, platform_module, method_name.c_str(),
                          &target_method);
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, CALL_JSB_ON_ARK_TS,
              [this, &method_name, &flow_id,
               &first_arg](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("module_name", module_name_);
                ctx.event()->add_debug_annotations("method_name", method_name);
                ctx.event()->add_debug_annotations("arg0", first_arg);
                ctx.event()->add_flow_ids(flow_id);
              });
  napi_value result;
  napi_call_function(env, platform_module, target_method, argv.size(),
                     argv.data(), &result);
  return result;
}

base::expected<std::unique_ptr<pub::Value>, std::string>
NativeModuleHarmony::InvokeMethod(const std::string& method_name,
                                  std::unique_ptr<pub::Value> args,
                                  size_t count,
                                  const piper::CallbackMap& callbacks) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, PUB_VALUE_TO_LEPUS_VALUE);
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  if (!lepus_value.IsArray()) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  std::string first_arg = "";
#if ENABLE_TRACE_PERFETTO
  if (lepus_value.Array().get()->size() > 0) {
    auto array = lepus_value.Array();
    if (array->get(0).IsString()) {
      first_arg = array->get(0).StdString();
    }
  }
#endif
  uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, HARMONY_INVOKE_NATIVE_MODULE,
              [this, &method_name, &flow_id,
               &first_arg](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("module_name", module_name_);
                ctx.event()->add_debug_annotations("method_name", method_name);
                ctx.event()->add_debug_annotations("arg0", first_arg);
                ctx.event()->add_flow_ids(flow_id);
              });
  auto delegate = delegate_.lock();
  if (!delegate) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  if (sendable_) {
    auto env = base::harmony::GetJSThreadNapiEnv();
    napi_value result = InvokeMethod(env, lepus_value, method_name, delegate,
                                     callbacks, flow_id, first_arg);
    lepus::Value lepus_result =
        base::NapiConvertHelper::ConvertToLepusValue(env, result);
    return std::make_unique<PubLepusValue>(std::move(lepus_result));
  }

  delegate->RunOnPlatformThread(
      [this, lepus_argv = std::move(lepus_value), method_name, delegate,
       callbacks = std::move(callbacks), flow_id = flow_id,
       first_arg = std::move(first_arg)]() mutable {
        LOGD("LynxModuleHarmony module: " << module_name_ << ", invoke method: "
                                          << method_name);
        InvokeMethod(main_env_, lepus_argv, method_name, delegate, callbacks,
                     flow_id, first_arg);
      });
  return std::unique_ptr<pub::Value>(nullptr);
}

void NativeModuleHarmony::Destroy() {
  LOGI("LynxModuleHarmony Destroy: " << module_name_);
}

// static
napi_value NativeModuleHarmony::Callback(napi_env env,
                                         napi_callback_info info) {
  static constexpr int STATIC_ARG_SIZE = 8;

  napi_value js_this;
  size_t argc = STATIC_ARG_SIZE;
  napi_value static_args[STATIC_ARG_SIZE] = {nullptr};
  napi_value* args = static_args;
  CallbackData* data = nullptr;
  napi_status status = napi_get_cb_info(env, info, &argc, args, &js_this,
                                        reinterpret_cast<void**>(&data));
  if (status != napi_ok) {
    return js_this;
  }
  auto delegate = data->delegate.lock();
  if (!delegate) {
    delete data;
    return js_this;
  }
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY_JSB, INVOKE_CALLBACK_ON_UI_THREAD,
      [&data](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("module_name", data->module_name);
        ctx.event()->add_debug_annotations("method_name", data->method_name);
        ctx.event()->add_debug_annotations("arg0", data->first_arg);
        ctx.event()->add_flow_ids(data->flow_id);
      });

  napi_value* dynamic_args = nullptr;
  if (argc > STATIC_ARG_SIZE) {
    // Use either a fixed-size array (on the stack) or a dynamically-allocated
    // array (on the heap) depending on the number of args.
    dynamic_args = new napi_value[argc];
    args = dynamic_args;
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  }
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, NAPI_VALUE_TO_LEPUS_VALUE);
  auto callback_args = lepus::CArray::Create();
  for (size_t i = 0; i < argc; i++) {
    callback_args->push_back(
        base::NapiConvertHelper::ConvertToLepusValue(env, args[i]));
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  data->callback->SetArgs(
      std::make_unique<PubLepusValue>(lepus::Value(callback_args)));
#if ENABLE_TRACE_PERFETTO
  data->callback->SetCallbackFlowId(data->flow_id);
#endif
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB,
      NATIVE_MODULE_HARMONY_CALLBACK_THREAD_SWITCH_START,
      [&data](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("module_name", data->module_name);
        ctx.event()->add_debug_annotations("method_name", data->method_name);
        ctx.event()->add_debug_annotations("arg0", data->first_arg);
      });
  delegate->InvokeCallback(data->callback);
  if (dynamic_args != nullptr) {
    delete[] dynamic_args;
  }
  delete data;
  return js_this;
}

}  // namespace harmony
}  // namespace lynx
