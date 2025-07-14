// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsvm/jsvm_runtime_wrapper.h"

#include <mutex>

#include "core/runtime/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace piper {
JSVMRuntimeInstance::~JSVMRuntimeInstance() {
  JSVM_CALL(OH_JSVM_CloseEnvScope(env_, env_scope_));
  JSVM_CALL(OH_JSVM_DestroyEnv(env_));
  JSVM_CALL(OH_JSVM_CloseVMScope(vm_, vm_scope_));
  JSVM_CALL(OH_JSVM_DestroyVM(vm_));
}

void JSVMRuntimeInstance::InitInstance() {
  static std::once_flag flag;
  std::call_once(flag, [this]() {
    LOGI("lynx JSVMRuntimeInstance::InitInstance");
    JSVM_InitOptions initOptions;
    memset(&initOptions, 0, sizeof(initOptions));
    JSVM_CALL(OH_JSVM_Init(&initOptions));

    JSVM_CreateVMOptions options;
    memset(&options, 0, sizeof(options));
    JSVM_CALL(OH_JSVM_CreateVM(&options, &vm_));

    JSVM_CALL(OH_JSVM_OpenVMScope(vm_, &vm_scope_));

    JSVM_CALL(OH_JSVM_CreateEnv(vm_, 0, nullptr, &env_));

    JSVM_CALL(OH_JSVM_OpenEnvScope(env_, &env_scope_));
  });
}
}  // namespace piper
}  // namespace lynx
