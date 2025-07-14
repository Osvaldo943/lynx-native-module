// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_runtime_proxy_jsvm.h"

#include <memory>

#include "core/runtime/jsi/jsvm/jsvm_runtime.h"
#include "third_party/napi/include/napi_env_harmony.h"

namespace lynx {
namespace piper {
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyJSVM::Create(
    const std::shared_ptr<JSVMContextWrapper>& context,
    runtime::TemplateDelegate* delegate) {
  return std::make_unique<NapiRuntimeProxyJSVM>(context, delegate);
}

NapiRuntimeProxyJSVM::NapiRuntimeProxyJSVM(
    const std::shared_ptr<JSVMContextWrapper>& context,
    runtime::TemplateDelegate* delegate)
    : NapiRuntimeProxy(delegate), jsvm_env_(context->getEnv()) {}

void NapiRuntimeProxyJSVM::Attach() { napi_attach_harmony(env_, jsvm_env_); }

void NapiRuntimeProxyJSVM::Detach() {
  NapiRuntimeProxy::Detach();
  napi_detach_harmony(env_);
}

std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyJSVMFactoryImpl::Create(
    std::shared_ptr<Runtime> runtime, runtime::TemplateDelegate* delegate) {
  LOGI("Creating napi proxy jsvm");
  auto jsvm_runtime = std::static_pointer_cast<JSVMRuntime>(runtime);
  auto jsvm_context = std::static_pointer_cast<JSVMContextWrapper>(
      jsvm_runtime->getSharedContext());
  auto proxy_jsvm = NapiRuntimeProxyJSVM::Create(jsvm_context, delegate);
  proxy_jsvm->SetJSRuntime(runtime);
  return proxy_jsvm;
}
}  // namespace piper
}  // namespace lynx
