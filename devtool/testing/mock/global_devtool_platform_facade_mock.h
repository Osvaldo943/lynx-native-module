// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_
#define DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_

#include <string>

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace testing {

class GlobalDevToolPlatformFacadeMock
    : public lynx::devtool::GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override {}
  void StopMemoryTracing() override {}
  lynx::trace::TraceController* GetTraceController() override {
    return nullptr;
  }
  lynx::trace::TracePlugin* GetFPSTracePlugin() override { return nullptr; }
  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override {
    return nullptr;
  }
  lynx::trace::TracePlugin* GetInstanceTracePlugin() override {
    return nullptr;
  }
};

}  // namespace testing

namespace devtool {
GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static lynx::testing::GlobalDevToolPlatformFacadeMock instance;
  return instance;
}
}  // namespace devtool

}  // namespace lynx
#endif  // DEVTOOL_TESTING_MOCK_GLOBAL_DEVTOOL_PLATFORM_FACADE_MOCK_H_
