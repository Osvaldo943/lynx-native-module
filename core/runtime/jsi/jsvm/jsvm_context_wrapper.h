// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_CONTEXT_WRAPPER_H_

#include <ark_runtime/jsvm_types.h>

#include <memory>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jsvm/jsvm_runtime_wrapper.h"

namespace lynx {
namespace piper {
class JSVMContextWrapper : public JSIContext {
 public:
  explicit JSVMContextWrapper(std::shared_ptr<VMInstance> vm);
  ~JSVMContextWrapper() override = default;

  JSVM_Env getEnv() const { return env_; };

 private:
  JSVM_Env env_ = nullptr;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_CONTEXT_WRAPPER_H_
