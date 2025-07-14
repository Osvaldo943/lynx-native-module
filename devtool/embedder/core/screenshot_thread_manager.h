// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_SCREENSHOT_THREAD_MANAGER_H_
#define DEVTOOL_EMBEDDER_CORE_SCREENSHOT_THREAD_MANAGER_H_

#include "base/include/fml/thread.h"
#include "base/include/thread/timed_task.h"

namespace lynx {
namespace devtool {
namespace ScreenshotThreadManager {

lynx::fml::RefPtr<lynx::fml::TaskRunner> GetScreenshotRunner();

lynx::base::TimedTaskManager& GetTimer();

}  // namespace ScreenshotThreadManager
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_SCREENSHOT_THREAD_MANAGER_H_
