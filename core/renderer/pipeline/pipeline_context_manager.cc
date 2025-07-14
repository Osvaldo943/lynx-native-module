// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_context_manager.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {

PipelineContextManager::PipelineContextManager(
    bool enable_unified_pixel_pipeline)
    : enable_unified_pixel_pipeline_(enable_unified_pixel_pipeline),
      current_version_(PipelineVersion::Create()) {}

PipelineContext* PipelineContextManager::CreateAndUpdateCurrentPipelineContext(
    const std::shared_ptr<PipelineOptions>& pipeline_options,
    bool is_major_updated) {
  // TODO(yangguangzhao): optimize this logic, maybe can move to observer.
  if (on_create_hook_) {
    on_create_hook_();
  }

  if (!enable_unified_pixel_pipeline_) {
    // Quick rejection for pixel pipeline.
    return nullptr;
  }

  if (pipeline_options->HeldByContext()) {
    return GetPipelineContextByVersion(*pipeline_options->version);
  }

  auto pipeline_context =
      PipelineContext::Create(current_version_, is_major_updated);
  current_version_ = pipeline_context->GetVersion();
  if (!pipeline_context) {
    LOGE("create pipeline context get nullptr");
    return nullptr;
  }
  pipeline_options->enable_unified_pixel_pipeline =
      enable_unified_pixel_pipeline_;

  // Set pipeline options to pipeline context, and mark options as held by
  // context.
  pipeline_context->SetOptions(pipeline_options);
  pipeline_options->version = &(pipeline_context->GetVersion());

  current_pipeline_context_ = pipeline_context.get();
  DCHECK(pipeline_contexts_.count(current_pipeline_context_->GetVersion()) ==
         0);
  pipeline_contexts_.emplace(pipeline_context->GetVersion(),
                             std::move(pipeline_context));

  return current_pipeline_context_;
}

PipelineContext* PipelineContextManager::GetPipelineContextByVersion(
    const PipelineVersion& version) const {
  if (auto it = pipeline_contexts_.find(version);
      it != pipeline_contexts_.end()) {
    return it->second.get();
  }

  LOGE("pipeline context not found by version: " << version.ToString())
  return nullptr;
}
}  // namespace tasm
}  // namespace lynx
