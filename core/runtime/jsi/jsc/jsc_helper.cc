//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsc/jsc_helper.h"

#include <array>
#include <unordered_map>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/jsc/jsc_exception.h"
#include "core/runtime/jsi/jsc/jsc_runtime.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace piper {
namespace detail {

JSCSymbolValue::JSCSymbolValue(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                               std::atomic<intptr_t>& counter, JSValueRef sym)
    : JSCObjectBase(ctx, jsc_runtime, sym, counter) {
  DCHECK(JSCHelper::smellsLikeES6Symbol(ctx_, sym_));
}

JSCStringValue::JSCStringValue(std::atomic<intptr_t>& counter, JSStringRef str)
    : str_(JSStringRetain(str))
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
      ,
      counter_(counter)
#endif
{
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
  counter_ += 1;
#endif
}

void JSCStringValue::invalidate() {
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
  counter_ -= 1;
#endif
  JSStringRelease(str_);
  delete this;
}

JSCObjectValue::JSCObjectValue(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                               std::atomic<intptr_t>& counter, JSObjectRef obj)
    : JSCObjectBase(ctx, jsc_runtime, obj, counter) {}

Runtime::PointerValue* JSCHelper::makeSymbolValue(
    JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
    std::atomic<intptr_t>& counter, JSValueRef sym) {
  return new JSCSymbolValue(ctx, jsc_runtime, counter, sym);
}

Runtime::PointerValue* JSCHelper::makeStringValue(
    std::atomic<intptr_t>& counter, JSStringRef str) {
  if (!str) {
    str = JSStringCreateWithUTF8CString("");
  }
  return new JSCStringValue(counter, str);
}

Runtime::PointerValue* JSCHelper::makeObjectValue(
    JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
    std::atomic<intptr_t>& counter, JSObjectRef obj) {
  if (!obj) {
    obj = JSObjectMake(ctx, nullptr, nullptr);
  }
  return new JSCObjectValue(ctx, jsc_runtime, counter, obj);
}

Value JSCHelper::createValue(JSCRuntime& rt, JSValueRef value) {
  JSGlobalContextRef ctx = rt.getContext();
  std::atomic<intptr_t>& counter = rt.objectCounter();
  if (!ctx) {
    return Value::null();
  }
  if (JSValueIsNumber(ctx, value)) {
    return Value(JSValueToNumber(ctx, value, nullptr));
  } else if (JSValueIsBoolean(ctx, value)) {
    return Value(JSValueToBoolean(ctx, value));
  } else if (JSValueIsNull(ctx, value)) {
    return Value(nullptr);
  } else if (JSValueIsUndefined(ctx, value)) {
    return Value();
  } else if (JSValueIsString(ctx, value)) {
    JSStringRef str = JSValueToStringCopy(ctx, value, nullptr);
    auto result = Value(JSCHelper::createString(counter, str));
    JSStringRelease(str);
    return result;
  } else if (JSValueIsObject(ctx, value)) {
    JSObjectRef objRef = JSValueToObject(ctx, value, nullptr);
    return Value(JSCHelper::createObject(ctx, &rt, counter, objRef));
  } else if (JSCHelper::smellsLikeES6Symbol(ctx, value)) {
    return Value(JSCHelper::createSymbol(ctx, &rt, counter, value));
  } else {
    int tag = JSValueGetType(ctx, value);
    LOGE("createValue failed type is unknown:" << tag);
    std::string msg =
        "createValue failed type is unknown:" + std::to_string(tag);
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(msg));
    return piper::Value();
  }
}

Symbol JSCHelper::createSymbol(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                               std::atomic<intptr_t>& counter, JSValueRef sym) {
  return Runtime::make<Symbol>(makeSymbolValue(ctx, jsc_runtime, counter, sym));
}

piper::String JSCHelper::createString(std::atomic<intptr_t>& counter,
                                      JSStringRef str) {
  return Runtime::make<piper::String>(makeStringValue(counter, str));
}

PropNameID JSCHelper::createPropNameID(std::atomic<intptr_t>& counter,
                                       JSStringRef str) {
  return Runtime::make<PropNameID>(makeStringValue(counter, str));
}

Object JSCHelper::createObject(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                               std::atomic<intptr_t>& counter,
                               JSObjectRef obj) {
  return Runtime::make<Object>(makeObjectValue(ctx, jsc_runtime, counter, obj));
}

JSObjectRef JSCHelper::createArrayBufferFromJS(JSCRuntime& rt,
                                               JSGlobalContextRef ctx,
                                               const uint8_t* bytes,
                                               size_t byte_length) {
  std::unique_ptr<char[]> base64_buf =
      std::make_unique<char[]>(modp_b64_encode_len(byte_length));
  size_t base64_len =
      modp_b64_encode(base64_buf.get(), (const char*)bytes, byte_length);
  if (base64_len == static_cast<size_t>(-1)) {
    return nullptr;
  }
  JSStringRef base64_str = JSStringCreateWithUTF8CString(base64_buf.get());
  JSObjectRef global = JSContextGetGlobalObject(ctx);
  JSObjectRef base64_to_ab = JSCHelper::getJSObject(
      rt, ctx, global,
      JSCHelper::getJSStringFromPool("__lynxBase64ToArrayBuffer"));
  if (!base64_to_ab) {
    return nullptr;
  }
  JSValueRef base64_str_value = JSValueMakeString(ctx, base64_str);
  JSValueRef exc = nullptr;
  JSValueRef array_buffer_value = JSObjectCallAsFunction(
      ctx, base64_to_ab, nullptr, 1, &base64_str_value, &exc);
  JSStringRelease(base64_str);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc) ||
      !array_buffer_value) {
    return nullptr;
  }
  JSObjectRef array_buffer = JSValueToObject(ctx, array_buffer_value, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc) || !array_buffer) {
    return nullptr;
  }
  return array_buffer;
}

JSValueRef JSCHelper::valueRef(JSGlobalContextRef ctx, JSCRuntime& rt,
                               const Value& value) {
  switch (value.kind()) {
    case Value::ValueKind::UndefinedKind:
      return JSValueMakeUndefined(ctx);
    case Value::ValueKind::NullKind:
      return JSValueMakeNull(ctx);
    case Value::ValueKind::BooleanKind:
      return JSValueMakeBoolean(ctx, value.getBool());
    case Value::ValueKind::NumberKind:
      return JSValueMakeNumber(ctx, value.getNumber());
    case Value::ValueKind::SymbolKind:
      return JSCHelper::symbolRef(value.getSymbol(rt));
    case Value::ValueKind::StringKind:
      return JSValueMakeString(ctx, JSCHelper::stringRef(value.getString(rt)));
    case Value::ValueKind::ObjectKind: {
      const JSCObjectValue* jsc_obj =
          static_cast<const JSCObjectValue*>(Runtime::getPointerValue(value));
      return jsc_obj->Get();
    }
  }
}

JSValueRef JSCHelper::symbolRef(const Symbol& sym) {
  return static_cast<const JSCSymbolValue*>(Runtime::getPointerValue(sym))
      ->sym_;
}

JSStringRef JSCHelper::stringRef(const piper::String& str) {
  return static_cast<const JSCStringValue*>(Runtime::getPointerValue(str))
      ->str_;
}

JSStringRef JSCHelper::stringRef(const PropNameID& str) {
  return static_cast<const JSCStringValue*>(Runtime::getPointerValue(str))
      ->str_;
}

JSObjectRef JSCHelper::objectRef(const Object& obj) {
  return static_cast<const JSCObjectValue*>(Runtime::getPointerValue(obj))
      ->Get();
}

std::string JSCHelper::JSStringToSTLString(JSStringRef str) {
  constexpr int kMaxStackBufferSize = 20;
  if (!str) {
    return "";
  }
  // Small string optimization: Avoid one heap allocation for strings that fit
  // in stack_buffer.size() bytes of UTF-8 (including the null terminator).
  std::array<char, kMaxStackBufferSize> stack_buffer;
  std::unique_ptr<char[]> heap_buffer;
  char* buffer;
  // NOTE: By definition, maxBytes >= 1 since the null terminator is included.
  size_t max_bytes = JSStringGetMaximumUTF8CStringSize(str);
  size_t str_len = JSStringGetLength(str);
  if (max_bytes <= 1 || str_len <= 0) {
    return "";
  }
  if (max_bytes <= stack_buffer.size()) {
    buffer = stack_buffer.data();
  } else {
    heap_buffer = std::make_unique<char[]>(max_bytes);
    buffer = heap_buffer.get();
  }
  size_t actual_bytes = JSStringGetUTF8CString(str, buffer, max_bytes);
  if (actual_bytes <= 0) {
    // Happens if max_bytes == 0 (never the case here) or if str contains
    // invalid UTF-16 data, since JSStringGetUTF8CString attempts a strict
    // conversion.
    // When converting an invalid string, JSStringGetUTF8CString writes a null
    // terminator before returning. So we can reliably treat our buffer as a C
    // string and return the truncated data to our caller. This is slightly
    // slower than if we knew the length (like below) but better than crashing.
    return std::string(buffer);
  }
  return std::string(buffer, actual_bytes - 1);
}

JSStringRef JSCHelper::getJSStringFromPool(std::string str) {
  static thread_local base::NoDestructor<
      std::unordered_map<std::string, JSStringRef>>
      js_string_pool;
  if (str.empty()) {
    return nullptr;
  }
  if (js_string_pool.get()->count(str)) {
    return js_string_pool.get()->at(str);
  } else {
    JSStringRef js_string = JSStringCreateWithUTF8CString(str.c_str());
    js_string_pool.get()->emplace(std::move(str), js_string);
    return js_string;
  }
}

JSObjectRef JSCHelper::getJSObject(JSCRuntime& rt, JSGlobalContextRef ctx,
                                   JSObjectRef obj, JSStringRef str) {
  JSValueRef exc = nullptr;
  JSValueRef property = JSObjectGetProperty(ctx, obj, str, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc) || !property) {
    return nullptr;
  }
  if (!JSValueIsObject(ctx, property)) {
    return nullptr;
  }
  JSObjectRef object = JSValueToObject(ctx, property, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc) || !object) {
    return nullptr;
  }
  return object;
}

bool JSCHelper::smellsLikeES6Symbol(JSGlobalContextRef ctx, JSValueRef ref) {
  if (__builtin_available(iOS 13.0, macOS 10.15, *)) {
    return JSValueIsSymbol(ctx, ref);
  } else {
    return (!JSValueIsObject(ctx, ref) &&
            JSValueGetType(ctx, ref) == kJSTypeObject);
  }
}

std::optional<Value> JSCHelper::call(JSGlobalContextRef ctx, JSCRuntime& rt,
                                     const Function& f, const Value& jsThis,
                                     const Value* args, size_t nArgs) {
  auto js_func = objectRef(f);
  if (!ctx || !rt.Valid() || !js_func) {
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
    LOGF("Error: " << __FILE__ << ":" << __LINE__ << ":"
                   << "JSCRuntime destroyed with a nullptr ctx.");
#endif
    LOGE("call after jsc destroy.");
    return std::nullopt;
  }
  JSValueRef exc = nullptr;
  auto converter =
      ArgsConverter<JSValueRef>(nArgs, args, [&ctx, &rt](const auto& value) {
        return JSCHelper::valueRef(ctx, rt, value);
      });
  auto res = JSObjectCallAsFunction(
      ctx, js_func,
      jsThis.isUndefined() ? nullptr : objectRef(jsThis.getObject(rt)), nArgs,
      converter, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc)) {
    return std::optional<Value>();
  }
  return createValue(rt, res);
}

std::optional<Value> JSCHelper::callAsConstructor(JSGlobalContextRef ctx,
                                                  JSCRuntime& rt,
                                                  const Function& f,
                                                  const Value* args,
                                                  size_t nArgs) {
  auto js_func = objectRef(f);
  if (!ctx || !rt.Valid() || !js_func) {
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
    LOGF("Error: " << __FILE__ << ":" << __LINE__ << ":"
                   << "JSCRuntime destroyed with a nullptr ctx.");
#endif
    LOGE("callAsConstructor after jsc destroy.");
    return std::nullopt;
  }
  JSValueRef exc = nullptr;
  auto converter =
      ArgsConverter<JSValueRef>(nArgs, args, [&ctx, &rt](const auto& value) {
        return JSCHelper::valueRef(ctx, rt, value);
      });
  auto res = JSObjectCallAsConstructor(ctx, js_func, nArgs, converter, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx, rt, exc)) {
    return std::optional<Value>();
  }
  return createValue(rt, res);
}

void JSCHelper::ThrowJsException(JSContextRef ctx,
                                 const JSINativeException& jsi_exception,
                                 JSValueRef* exception) {
  DCHECK(!jsi_exception.message().empty());
  auto message_js_str =
      JSStringCreateWithUTF8CString(jsi_exception.message().c_str());
  auto stack_js_str =
      JSStringCreateWithUTF8CString(jsi_exception.stack().c_str());

  JSGlobalContextRef global_ctx = JSContextGetGlobalContext(ctx);
  auto message_value = JSValueMakeString(global_ctx, message_js_str);
  auto stack_value = JSValueMakeString(global_ctx, stack_js_str);
  JSValueRef error = JSObjectMakeError(ctx, 1, &message_value, exception);
  if (jsi_exception.IsJSError()) {
    auto name_js_str = JSStringCreateWithUTF8CString(jsi_exception.name());
    auto name_value = JSValueMakeString(global_ctx, name_js_str);
    JSObjectSetProperty(ctx, (JSObjectRef)error, getJSStringFromPool("name"),
                        name_value, kJSPropertyAttributeNone, NULL);
    JSObjectSetProperty(ctx, (JSObjectRef)error, getJSStringFromPool("stack"),
                        stack_value, kJSPropertyAttributeNone, NULL);
  } else {
    JSObjectSetProperty(ctx, (JSObjectRef)error, getJSStringFromPool("cause"),
                        stack_value, kJSPropertyAttributeNone, NULL);
  }

  *exception = error;
  JSStringRelease(message_js_str);
  JSStringRelease(stack_js_str);
}

}  // namespace detail
}  // namespace piper
}  // namespace lynx
