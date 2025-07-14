// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/jsc/jsc_context_group_wrapper.h"

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace piper {

JSCContextGroupWrapper::~JSCContextGroupWrapper() {
  LOGI("~JSCContextGroupWrapper " << this);
  if (group_ != nullptr) {
    LOGI("JSContextGroupRelease:" << group_);
    JSContextGroupRelease(group_);
    group_ = nullptr;
  }
}

void JSCContextGroupWrapper::InitContextGroup() {
  LOGI("JSContextGroupCreate");
  group_ = JSContextGroupCreate();
}

}  // namespace piper
}  // namespace lynx
