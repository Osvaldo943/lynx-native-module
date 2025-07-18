// Inspired by S.js by Adam Haile, https://github.com/adamhaile/S
/**
The MIT License (MIT)

Copyright (c) 2017 Adam Haile

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/signal_context.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/scope.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

SignalContext::SignalContext() {}

void SignalContext::PushScope(BaseScope* scope) {
  scope_stack_.emplace_back(scope);
}

void SignalContext::PopScope() { scope_stack_.pop_back(); }

BaseScope* SignalContext::GetTopScope() {
  if (scope_stack_.empty()) {
    return nullptr;
  }
  return scope_stack_.back();
}

void SignalContext::PushComputation(Computation* computation) {
  computation_stack_.emplace_back(computation);
}

void SignalContext::PopComputation() { computation_stack_.pop_back(); }

void SignalContext::MarkUnTrack(bool enable_un_track) {
  if (enable_un_track) {
    PushComputation(nullptr);
  } else {
    PopComputation();
  }
}

Computation* SignalContext::GetTopComputation() {
  if (computation_stack_.empty()) {
    return nullptr;
  }
  return computation_stack_.back();
}

void SignalContext::RunUpdates(std::function<void()>&& func) {
  if (memo_computation_list_ptr_ != nullptr) {
    func();
    return;
  }

  EnsureMemoComputationList();

  bool wait = false;
  if (pure_computation_list_ptr_ != nullptr) {
    wait = true;
  } else {
    EnsurePureComputationList();
  }

  exec_count_++;
  func();
  CompleteUpdates(wait);
  return;
}

void SignalContext::CompleteUpdates(bool wait) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SIGNAL_CONTEXT_COMPLETE_UPDATES);

  if (memo_computation_list_ptr_ != nullptr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, SIGNAL_CONTEXT_COMPLETE_UPDATES_UPDATES,
                [this](lynx::perfetto::EventContext ctx) {
                  auto event = ctx.event();
                  auto* tagInfo = event->add_debug_annotations();
                  tagInfo->set_name("count");
                  tagInfo->set_int_value(memo_computation_list_ptr_->size());
                });
    auto u = memo_computation_list_ptr_;
    RunComputation(std::move(u));
    memo_computation_list_ptr_ = nullptr;
  }

  if (wait) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, SIGNAL_CONTEXT_COMPLETE_UPDATES_EFFECTS,
              [this](lynx::perfetto::EventContext ctx) {
                auto event = ctx.event();
                auto* tagInfo = event->add_debug_annotations();
                tagInfo->set_name("count");
                tagInfo->set_int_value(pure_computation_list_ptr_ != nullptr
                                           ? pure_computation_list_ptr_->size()
                                           : 0);
              });
  auto e = pure_computation_list_ptr_;
  pure_computation_list_ptr_ = nullptr;

  if (e != nullptr && !e->empty()) {
    RunUpdates([this, list = std::move(e)]() mutable {
      RunComputation(std::move(list));
    });
  }
}

void SignalContext::EnqueueComputation(Computation* computation) {
  if (computation->GetScopeType() == ScopeType::kPureComputation) {
    EnsurePureComputationList();
    pure_computation_list_ptr_->emplace_back(
        fml::RefPtr<Computation>(computation));
  } else if (computation->GetScopeType() == ScopeType::kMemoComputation) {
    EnsureMemoComputationList();
    memo_computation_list_ptr_->emplace_back(
        fml::RefPtr<Computation>(computation));
  }
}

bool SignalContext::IsScopeActiveComputation(BaseScope* scope) {
  return scope != nullptr && scope->GetScopeType() != ScopeType::kPureScope &&
         scope->GetUpdatedTime() < exec_count_ &&
         scope->GetState() != ScopeState::kStateNone;
}

void SignalContext::RunComputation(Computation* computation) {
  if (computation->GetState() == ScopeState::kStateNone) {
    return;
  }

  if (computation->GetState() == ScopeState::kStatePending) {
    computation->LookUpstream(computation);
    return;
  }

  std::list<fml::RefPtr<Computation>> ancestors;
  ancestors.push_back(fml::RefPtr<Computation>(computation));

  auto node = computation->GetOwner();
  while (IsScopeActiveComputation(node)) {
    ancestors.push_front(
        fml::RefPtr<Computation>(static_cast<Computation*>(node)));
    node = node->GetOwner();
  }

  for (auto computation : ancestors) {
    if (computation->GetState() == ScopeState::kStateStale) {
      UpdateComputation(computation.get());
    } else if (computation->GetState() == ScopeState::kStatePending) {
      auto updates = memo_computation_list_ptr_;
      memo_computation_list_ptr_ = nullptr;
      RunUpdates([computation, &ancestors]() {
        computation->LookUpstream(ancestors.back().get());
      });
      memo_computation_list_ptr_ = updates;
    }
  }
}

void SignalContext::RunComputation(
    std::shared_ptr<std::list<fml::RefPtr<Computation>>>&& list) {
  for (auto computation : *list) {
    RunComputation(computation.get());
  }
}

void SignalContext::UpdateComputation(Computation* computation) {
  computation->CleanUp();

  PushScope(computation);
  PushComputation(computation);

  computation->Invoke(exec_count_);

  PopComputation();
  PopScope();
}

void SignalContext::EnsurePureComputationList() {
  if (pure_computation_list_ptr_ != nullptr) {
    return;
  }
  pure_computation_list_ptr_ =
      std::make_shared<std::list<fml::RefPtr<Computation>>>();
}

void SignalContext::EnsureMemoComputationList() {
  if (memo_computation_list_ptr_ != nullptr) {
    return;
  }
  memo_computation_list_ptr_ =
      std::make_shared<std::list<fml::RefPtr<Computation>>>();
}

void SignalContext::WillDestroy() {
  for (auto scope : scope_set_) {
    scope->WillDestroy();
  }
}

void SignalContext::RecordScope(Scope* scope) { scope_set_.insert(scope); }

void SignalContext::EraseScope(Scope* scope) { scope_set_.erase(scope); }

}  // namespace tasm
}  // namespace lynx
