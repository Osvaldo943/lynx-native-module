// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/screenshot_thread_manager.h"

#include "base/include/no_destructor.h"

namespace lynx {
namespace devtool {
namespace ScreenshotThreadManager {

lynx::fml::RefPtr<lynx::fml::TaskRunner> GetScreenshotRunner() {
  static lynx::base::NoDestructor<lynx::fml::Thread> screenshot_thread(
      "Lynx_SnapshotThread");
  return screenshot_thread->GetTaskRunner();
}

lynx::base::TimedTaskManager& GetTimer() {
  static lynx::base::NoDestructor<lynx::base::TimedTaskManager> timer;
  return *timer;
}

}  // namespace ScreenshotThreadManager
}  // namespace devtool
}  // namespace lynx
