// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_HARMONY_H_
#define BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_HARMONY_H_

#include <node_api.h>

namespace lynx {
namespace trace {

class TraceControllerHarmony {
 public:
  static napi_value Init(napi_env env, napi_value exports);

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value StartTracing(napi_env env, napi_callback_info info);
  static napi_value StopTracing(napi_env env, napi_callback_info info);
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_HARMONY_H_
