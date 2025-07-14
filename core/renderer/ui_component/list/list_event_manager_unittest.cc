// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/ui_component/list/list_event_manager.h"

#include <algorithm>
#include <memory>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_component/list/default_list_adapter.h"
#include "core/renderer/ui_component/list/linear_layout_manager.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/testing/mock_diff_result.h"
#include "core/renderer/ui_component/list/testing/mock_list_element.h"
#include "core/renderer/ui_component/list/testing/utils.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ListMockTasmDelegate : public lynx::tasm::test::MockTasmDelegate {
 public:
  MOCK_METHOD(void, SendNativeCustomEvent,
              (const std::string& name, int tag,
               const lepus::Value& param_value, const std::string& param_name),
              (override));
};

class ListEventManagerTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> element_manager_;
  std::shared_ptr<::testing::NiceMock<ListMockTasmDelegate>> tasm_mediator_;
  fml::RefPtr<list::MockListElement> list_element_ref_;
  std::unique_ptr<ListContainerImpl> list_container_;
  ListAdapter* list_adapter_;
  ListChildrenHelper* list_children_helper_;
  ListLayoutManager* list_layout_manager_;
  ListEventManager* list_event_manager_;

 public:
  void ExpectCallSendNativeCustomEvent(const std::string& event_name,
                                       const int impl_id, int times) {
    EXPECT_CALL(
        *tasm_mediator_,
        SendNativeCustomEvent(::testing::Eq(event_name), ::testing::Eq(impl_id),
                              ::testing::_, ::testing::Eq("detail")))
        .Times(times);
  }

  void MockScrollAndLayoutState(int list_main_axis_size, int content_offset) {
    LinearLayoutManager* linear_layout_manager =
        static_cast<LinearLayoutManager*>(list_layout_manager_);
    // Layout all item holders.
    linear_layout_manager->LayoutInvalidItemHolder(0);
    // Calculate content size.
    linear_layout_manager->content_size_ =
        linear_layout_manager->GetTargetContentSize();
    linear_layout_manager->content_offset_ = content_offset;
    list_element_ref_->height_ = list_main_axis_size;
    list_children_helper_->UpdateOnScreenChildren(
        linear_layout_manager->list_orientation_helper_.get(), content_offset);
  }

 protected:
  ListEventManagerTest() = default;
  ~ListEventManagerTest() override = default;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ =
        std::make_shared<::testing::NiceMock<ListMockTasmDelegate>>();
    element_manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    element_manager_->SetConfig(config);

    lepus::Value component_at_index;
    lepus::Value enqueue_component;
    lepus::Value component_at_indexes;
    list_element_ref_ =
        fml::AdoptRef<list::MockListElement>(new list::MockListElement(
            element_manager_.get(), "list", component_at_index,
            enqueue_component, component_at_indexes));
    list_element_ref_->width_ = 400;
    list_element_ref_->height_ = 400;
    list_container_ =
        std::make_unique<ListContainerImpl>(list_element_ref_.get());
    list_adapter_ = list_container_->list_adapter();
    list_children_helper_ = list_container_->list_children_helper();
    list_event_manager_ = list_container_->list_event_manager();
    list_layout_manager_ = list_container_->list_layout_manager();
    list::DiffResult diff_result{
        .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                      "I_8", "J_9"},
        .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        .estimated_height_pxs = {100, 100, 100, 100, 100, 100, 100, 100, 100,
                                 100},
    };
    list_adapter_->UpdateDataSource(
        lepus_value(diff_result.GenerateDiffResult()));
    list_adapter_->UpdateItemHolderToLatest(list_children_helper_);
  }
};  // ListEventManagerTest

TEST_F(ListEventManagerTest, AddEvent) {
  list_event_manager_->AddEvent(list::kScroll);
  list_event_manager_->AddEvent(list::kScrollToUpper);
  list_event_manager_->AddEvent(list::kScrollToUpperEdge);
  list_event_manager_->AddEvent(list::kScrollToLower);
  list_event_manager_->AddEvent(list::kScrollToLowerEdge);
  list_event_manager_->AddEvent(list::kScrollToNormalState);
  list_event_manager_->AddEvent(list::kLayoutComplete);
  const auto& event_map = list_event_manager_->events_;
  EXPECT_TRUE(event_map.find(list::kScroll) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kScrollToUpper) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kScrollToUpperEdge) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kScrollToLower) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kScrollToLowerEdge) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kScrollToNormalState) != event_map.end());
  EXPECT_TRUE(event_map.find(list::kLayoutComplete) != event_map.end());
}

TEST_F(ListEventManagerTest, SendLayoutCompleteEvent) {
  // 1. No bind layout complete event.
  ExpectCallSendNativeCustomEvent(list::kLayoutComplete,
                                  list_element_ref_->impl_id(), 0);
  list_layout_manager_->SendLayoutCompleteEvent();

  // 2. Bind layout complete event.
  ExpectCallSendNativeCustomEvent(list::kLayoutComplete,
                                  list_element_ref_->impl_id(), 1);
  list_event_manager_->AddEvent(list::kLayoutComplete);
  list_layout_manager_->SendLayoutCompleteEvent();

  // 3. If is non smooth scroll, we should no send layout complete event.
  list_layout_manager_->is_non_smooth_scroll_ = true;
  ExpectCallSendNativeCustomEvent(list::kLayoutComplete,
                                  list_element_ref_->impl_id(), 0);
  list_layout_manager_->SendLayoutCompleteEvent();
}

TEST_F(ListEventManagerTest, DetectScrollToThresholdAndSend0) {
  LinearLayoutManager* linear_layout_manager =
      static_cast<LinearLayoutManager*>(list_layout_manager_);
  list_event_manager_->AddEvent(list::kScrollToUpper);
  list_event_manager_->AddEvent(list::kScrollToUpperEdge);
  list_event_manager_->AddEvent(list::kScrollToLower);
  list_event_manager_->AddEvent(list::kScrollToLowerEdge);
  LinearLayoutManager::LayoutState layout_state;
  // list_size: 2000, content_size: 1000, content_offset: 0, original_offset: 0
  // list_size > content_size
  int content_offset = 0;
  int original_offset = 0;
  int list_size = 2000;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 1);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kBothEdge);
}

TEST_F(ListEventManagerTest, DetectScrollToThresholdAndSend1) {
  LinearLayoutManager* linear_layout_manager =
      static_cast<LinearLayoutManager*>(list_layout_manager_);
  list_event_manager_->AddEvent(list::kScrollToUpper);
  list_event_manager_->AddEvent(list::kScrollToUpperEdge);
  list_event_manager_->AddEvent(list::kScrollToLower);
  list_event_manager_->AddEvent(list::kScrollToLowerEdge);
  LinearLayoutManager::LayoutState layout_state;
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kMiddle);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: 0
  int content_offset = 0;
  int original_offset = 0;
  int list_size = 500;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 0);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kUpper);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: 0
  content_offset = 500;
  original_offset = 500;
  list_size = 500;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 1);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kLower);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: -20
  // scroll to bounce area
  content_offset = 0;
  original_offset = -20;
  list_size = 500;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 0);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kUpper);
  // bounce back to 0
  content_offset = 0;
  original_offset = 0;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 0);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kUpper);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: 520
  // scroll to bounce area
  content_offset = 500;
  original_offset = 520;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 1);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 0);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kLower);
  // bounce back to 500
  content_offset = 500;
  original_offset = 500;
  MockScrollAndLayoutState(list_size, content_offset);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpper,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToUpperEdge,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLower,
                                  list_element_ref_->impl_id(), 0);
  ExpectCallSendNativeCustomEvent(list::kScrollToLowerEdge,
                                  list_element_ref_->impl_id(), 1);
  linear_layout_manager->OnScrollAfter(layout_state, original_offset);
  EXPECT_TRUE(list_event_manager_->previous_scroll_state_ ==
              list::ListScrollState::kLower);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
