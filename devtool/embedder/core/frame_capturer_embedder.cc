// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/frame_capturer_embedder.h"

#include <chrono>
#include <utility>

#include "base/include/log/logging.h"
#include "devtool/embedder/core/screenshot_timer_trigger_embedder.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {
namespace devtool {

FrameCapturerEmbedder::FrameCapturerEmbedder() : snapshot_cache_("") {}

void FrameCapturerEmbedder::Init(
    const std::shared_ptr<FrameCapturerEmbedder>& sp_self) {
  std::weak_ptr<FrameCapturerEmbedder> weak_self = sp_self;
  screenshot_trigger_ =
      std::make_shared<ScreenshotTimerTriggerEmbedder>([weak_self] {
        auto sp_self = weak_self.lock();
        if (sp_self) {
          sp_self->Screenshot();
        }
      });
}

void FrameCapturerEmbedder::SetDelegate(
    const std::shared_ptr<FrameCapturerEmbedderDelegate>& delegate) {
  delegate_ = delegate;
}

void FrameCapturerEmbedder::StartFrameViewTrace() {
  CHECK_NULL_AND_LOG_RETURN(screenshot_trigger_, "screenshot_trigger_ is null");
  screenshot_trigger_->Start();
}

void FrameCapturerEmbedder::PauseFrameViewTrace() {
  if (screenshot_trigger_) {
    screenshot_trigger_->Pause();
  }
  snapshot_cache_.clear();
}

void FrameCapturerEmbedder::ContinueFrameViewTrace() {
  CHECK_NULL_AND_LOG_RETURN(screenshot_trigger_, "screenshot_trigger_ is null");
  screenshot_trigger_->Continue();
}

void FrameCapturerEmbedder::StopFrameViewTrace() {
  if (screenshot_trigger_) {
    // trigger will be stop when it deconstructing
    screenshot_trigger_->Pause();
  }
  snapshot_cache_.clear();
}

void FrameCapturerEmbedder::Screenshot() {
  auto sp_delegate = delegate_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp_delegate, "delegate_ is null");
  sp_delegate->TakeSnapshotAsync(
      [this, sp_delegate](const std::string& snapshot, float timestamp,
                          float device_width, float device_height,
                          float page_scale_factor) {
        if (snapshot.empty()) {
          return;
        }
        if (snapshot_cache_.empty() || snapshot_cache_.compare(snapshot)) {
          snapshot_cache_ = std::move(snapshot);
          devtool::ScreenMetadata meta_data;
          meta_data.timestamp_ = timestamp;
          meta_data.device_width_ = device_width;
          meta_data.device_height_ = device_height;
          meta_data.page_scale_factor_ = page_scale_factor;
          sp_delegate->OnNewSnapshot(snapshot_cache_, meta_data);
        }
      });
}

}  // namespace devtool
}  // namespace lynx
