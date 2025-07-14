// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_SCREENSHOT_TIMER_TRIGGER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_SCREENSHOT_TIMER_TRIGGER_EMBEDDER_H_

#include <functional>
#include <memory>

namespace lynx {
namespace devtool {

using ScreenshotCallback = std::function<void()>;

class ScreenshotTimerTriggerEmbedder
    : public std::enable_shared_from_this<ScreenshotTimerTriggerEmbedder> {
 public:
  explicit ScreenshotTimerTriggerEmbedder(ScreenshotCallback callback);
  ~ScreenshotTimerTriggerEmbedder();
  void Start();
  void Pause();
  void Continue();

 private:
  ScreenshotCallback callback_;
  uint32_t task_id_ = 0;

  bool is_running_ = false;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_SCREENSHOT_TIMER_TRIGGER_EMBEDDER_H_
