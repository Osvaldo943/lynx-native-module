// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_

#include <ark_runtime/jsvm.h>

#include <cstdlib>

#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsvm/jsvm_helper.h"

namespace lynx {
namespace piper {
#define JSVM_CALL(status)                       \
  if (status != JSVM_Status::JSVM_OK) {         \
    LOGE("jsvm call failed status:" << status); \
  }

#define JSVM_CALL_RETURN(status, ret)           \
  if (status != JSVM_Status::JSVM_OK) {         \
    LOGE("jsvm call failed status:" << status); \
    return ret;                                 \
  }

class HandleScopeWrapper {
 public:
  explicit HandleScopeWrapper(JSVM_Env env) : env(env) {
    JSVM_CALL(OH_JSVM_OpenHandleScope(env, &handleScope));
  }

  ~HandleScopeWrapper() {
    JSVM_CALL(OH_JSVM_CloseHandleScope(env, handleScope));
  }

  HandleScopeWrapper(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper& operator=(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper(HandleScopeWrapper&&) = delete;
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;

 protected:
  JSVM_Env env;
  JSVM_HandleScope handleScope;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_
