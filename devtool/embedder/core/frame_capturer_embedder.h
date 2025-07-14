// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_FRAME_CAPTURER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_FRAME_CAPTURER_EMBEDDER_H_

#include <functional>
#include <memory>
#include <string>

#include "core/public/ui_delegate.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {
namespace devtool {

class FrameCapturerEmbedderDelegate;
class ScreenshotTimerTriggerEmbedder;

class FrameCapturerEmbedder {
 public:
  FrameCapturerEmbedder();
  virtual ~FrameCapturerEmbedder() = default;
  void Init(const std::shared_ptr<FrameCapturerEmbedder>& sp_self);
  void StartFrameViewTrace();
  void ContinueFrameViewTrace();
  void PauseFrameViewTrace();
  void StopFrameViewTrace();
  void Screenshot();
  void SetDelegate(
      const std::shared_ptr<FrameCapturerEmbedderDelegate>& delegate);

 protected:
  std::weak_ptr<FrameCapturerEmbedderDelegate> delegate_;

  std::string snapshot_cache_;
  std::shared_ptr<ScreenshotTimerTriggerEmbedder> screenshot_trigger_;
};

class FrameCapturerEmbedderDelegate {
 public:
  FrameCapturerEmbedderDelegate() = default;
  virtual ~FrameCapturerEmbedderDelegate() = default;
  virtual void OnNewSnapshot(const std::string& data,
                             const devtool::ScreenMetadata&) = 0;
  virtual void OnFrameChanged() = 0;

  virtual void TakeSnapshotAsync(
      tasm::TakeSnapshotCompletedCallback callback) = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_FRAME_CAPTURER_EMBEDDER_H_
