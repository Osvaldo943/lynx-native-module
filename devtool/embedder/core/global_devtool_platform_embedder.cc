// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/global_devtool_platform_embedder.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/trace/native/trace_controller.h"
#endif
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace devtool {

class GlobalDevtoolPlatformCommon
    : public lynx::devtool::GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override {
    GlobalDevtoolPlatformEmbedder::StartMemoryTracing();
  }

  void StopMemoryTracing() override {
    GlobalDevtoolPlatformEmbedder::StopMemoryTracing();
  }

  lynx::trace::TraceController* GetTraceController() override {
    return lynx::trace::GetTraceControllerInstance();
  }

  lynx::trace::TracePlugin* GetFPSTracePlugin() override { return nullptr; }

  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override {
    return nullptr;
  }

  lynx::trace::TracePlugin* GetInstanceTracePlugin() override {
    return nullptr;
  }

  std::string GetLynxVersion() override {
    // TODO(suguannan.906): implement GetLynxVersion later
    return "";
  }
};

void GlobalDevtoolPlatformEmbedder::StartMemoryTracing() {}

void GlobalDevtoolPlatformEmbedder::StopMemoryTracing() {}

GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static GlobalDevtoolPlatformCommon instance;
  return instance;
}
}  // namespace devtool

}  // namespace lynx
