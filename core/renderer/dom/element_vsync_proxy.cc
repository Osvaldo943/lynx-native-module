// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_vsync_proxy.h"

#include <memory>
#include <set>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace tasm {

ElementVsyncProxy::ElementVsyncProxy(
    ElementManager *element_manager,
    const std::shared_ptr<base::VSyncMonitor> &vsync_monitor)
    : element_manager_(element_manager), vsync_monitor_(vsync_monitor){};

void ElementVsyncProxy::TickAllElement(fml::TimePoint &frame_time) {
  timing::LongTaskMonitor::Scope longTaskScope(
      element_manager_->GetPageOptions(), timing::kAnimationTask,
      timing::kTaskNameAnimationVSyncTickAllElement);
  // preferred_fps_ here won't be zero.
  if (preferred_fps_ == kPreferredFpsLow) {
    if (frame_time - last_tick_time() >= LOW_FRAME_DURATION) {
      element_manager_->TickAllElement(frame_time);
      set_last_tick_time(frame_time);
    } else {
      RequestNextFrame();
    }
  } else {
    element_manager_->TickAllElement(frame_time);
    set_last_tick_time(frame_time);
  }
}

void ElementVsyncProxy::SetPreferredFps(const std::string &preferred_fps) {
  set_preferred_fps(preferred_fps);
  if (preferred_fps == kPreferredFpsHigh) {
    report::FeatureCounter::Instance()->Count(
        report::LynxFeature::CPP_ENABLE_HIGH_REFRESH_RATE);
    vsync_monitor_->SetHighRefreshRate();
  }
}

// The first animation starts an infinite loop.
void ElementVsyncProxy::RequestNextFrame() {
  if (element_manager_->IsPause()) {
    return;
  }
  if (!has_requested_next_frame_ && vsync_monitor_) {
    std::weak_ptr<ElementVsyncProxy> weak_ptr{shared_from_this()};
    vsync_monitor_->ScheduleVSyncSecondaryCallback(
        reinterpret_cast<uintptr_t>(this),
        [weak_ptr, instance_id = element_manager_->GetInstanceId()](
            int64_t frame_start, int64_t frame_end) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_VSYNC_PROXY_FRAME_TIME,
                      [instance_id](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            INSTANCE_ID, std::to_string(instance_id));
                      });
          // TODO(WUJINTIAN): Access animation vsync proxy and element manager
          // through engine actor, instead of accessing them directly.
          auto shared_ptr = weak_ptr.lock();
          if (shared_ptr != nullptr) {
            shared_ptr->MarkNextFrameHasArrived();
            fml::TimePoint frame_time = fml::TimePoint::FromEpochDelta(
                fml::TimeDelta::FromNanoseconds(frame_start));
            shared_ptr->TickAllElement(frame_time);
          }
        },
        LynxEnv::GetInstance().EnableAnimationVsyncOnUIThread());
    has_requested_next_frame_ = true;
  }
}

}  // namespace tasm
}  // namespace lynx
