// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <objc/message.h>

#include "base/include/debug/backtrace.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/ui_wrapper/common/ios/platform_extra_bundle_darwin.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#include "core/renderer/utils/ios/text_utils_ios.h"
#include "core/runtime/bindings/jsi/modules/ios/lynx_module_darwin.h"
#include "core/services/feature_count/feature.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/value_impl_lepus.h"

#import <Lynx/AbsLynxUIScroller.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxNewGestureDelegate.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxTemplateBundle+Converter.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIImage.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/UIDevice+Lynx.h>
#import "LynxCallStackUtil.h"
#import "LynxEnv+Internal.h"
#import "LynxPerformanceController.h"
#import "LynxTemplateData+Converter.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTimingConstants.h"
#import "LynxTouchHandler+Internal.h"
#import "LynxUI+Gesture.h"
#import "LynxUI+Private.h"
#import "LynxUIOwner+Private.h"

namespace lynx {
namespace tasm {

// LEFT,TOP,RIGHT,BOTTOM

enum BoxModelOffset {
  PAD_LEFT = 0,
  PAD_TOP,
  PAD_RIGHT,
  PAD_BOTTOM,
  BORDER_LEFT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  MARGIN_LEFT,
  MARGIN_TOP,
  MARGIN_RIGHT,
  MARGIN_BOTTOM,
  LAYOUT_LEFT,
  LAYOUT_TOP,
  LAYOUT_RIGHT,
  LAYOUT_BOTTOM
};

namespace {

template <typename F>
void ExecuteSafely(const F& func) {
  @try {
    func();
  } @catch (NSException* e) {
    std::string msg = [[NSString stringWithFormat:@"%@:%@", [e name], [e reason]] UTF8String];
    if (LYNX_ERROR(error::E_EXCEPTION_PLATFORM, msg, "")) {
      auto& instance = lynx::base::ErrorStorage::GetInstance();
      std::string stack;
      NSString* rawStack = [LynxCallStackUtil getCallStack:e];
      stack = lynx::base::debug::GetBacktraceInfo(stack);
      instance.AddCustomInfoToError("error_stack", stack);
      if (rawStack) {
        instance.AddCustomInfoToError("raw_stack", rawStack.UTF8String);
      }
      NSDictionary* info = [e userInfo];
      NSDictionary* customInfo = [info objectForKey:@"LynxErrorCustomInfo"];
      if (customInfo) {
        for (NSString* key in customInfo) {
          instance.AddCustomInfoToError([key UTF8String],
                                        [[customInfo objectForKey:key] UTF8String]);
        }
      }
    }
  }
}

}  // namespace

void PaintingContextDarwinRef::InsertPaintingNode(int parent, int child, int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_INSERT_PAINTING_TASK);

  [uiOwner_ insertNode:child toParent:parent atIndex:index];
}

void PaintingContextDarwinRef::RemovePaintingNode(int parent, int child, int index, bool is_move) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_REMOVE_PAINTING_TASK);

  [uiOwner_ detachNode:child];
}

void PaintingContextDarwinRef::DestroyPaintingNode(int parent, int child, int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_DESTORY_PAINTING_TASK);

  [uiOwner_ recycleNode:child];
}

void PaintingContextDarwinRef::UpdateScrollInfo(int32_t container_id, bool smooth,
                                                float estimated_offset, bool scrolling) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_SCROLL_INFO_TASK);

  [uiOwner_ updateScrollInfo:container_id
             estimatedOffset:estimated_offset
                      smooth:smooth
                   scrolling:scrolling];
}

void PaintingContextDarwinRef::SetGestureDetectorState(int64_t idx, int32_t gesture_id,
                                                       int32_t state) {
  // Find the LynxUI associated with the given index (sign).
  LynxUI* ui = [uiOwner_ findUIBySign:(int)idx];

  // Check if the LynxUI exists and conforms to the LynxNewGestureDelegate protocol.
  // Set gesture detector state on the LynxUI.
  // state: 1 - Active, 2 - Fail, 3 - End
  [ui setGestureDetectorState:gesture_id state:(LynxGestureState)state];
}

void PaintingContextDarwinRef::UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                                                       std::vector<int32_t> remove_ids) {
  if (ready_ids.empty() && remove_ids.empty()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_NODE_READY_PATCHING);

  for (const auto& tag : ready_ids) {
    [uiOwner_ onNodeReady:tag];
  }
  for (const auto& tag : remove_ids) {
    [uiOwner_ onNodeRemoved:tag];
  }
}

void PaintingContextDarwinRef::UpdateNodeReloadPatching(std::vector<int32_t> reload_ids) {
  if (reload_ids.empty()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_NODE_RELOAD_PATCHING);

  for (const auto& tag : reload_ids) {
    [uiOwner_ onNodeReload:tag];
  }
}

void PaintingContextDarwinRef::UpdateEventInfo(bool has_touch_pseudo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_EVENT_INFO);

  [uiOwner_.uiContext.eventHandler.touchRecognizer setEnableTouchPseudo:has_touch_pseudo];
}

void PaintingContextDarwinRef::ListReusePaintingNode(int sign, const std::string& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_LIST_REUSE_PAINTING_NODE);

  [uiOwner_ listWillReuseNode:sign withItemKey:[NSString stringWithUTF8String:item_key.c_str()]];
}

void PaintingContextDarwinRef::ListCellWillAppear(int sign, const std::string& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_LIST_CELL_WILL_APPEAR);

  auto itemKey = [NSString stringWithUTF8String:item_key.c_str()];
  [uiOwner_ listCellWillAppear:sign withItemKey:itemKey];
}

void PaintingContextDarwinRef::ListCellDisappear(int sign, bool isExist,
                                                 const std::string& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_LIST_CELL_DISAPPEAR);

  auto itemKey = [NSString stringWithUTF8String:item_key.c_str()];
  [uiOwner_ ListCellDisappear:sign exist:isExist withItemKey:itemKey];
}

void PaintingContextDarwinRef::UpdateContentOffsetForListContainer(int32_t container_id,
                                                                   float content_size,
                                                                   float delta_x, float delta_y,
                                                                   bool is_init_scroll_offset,
                                                                   bool from_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_OFFSET_FOR_LIST);

  [uiOwner_ updateContentOffsetForListContainer:container_id
                                    contentSize:content_size
                                         deltaX:delta_x
                                         deltaY:delta_y];
}

void PaintingContextDarwinRef::InsertListItemPaintingNode(int32_t list_id, int32_t child_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_INSERT_LIST_PAINTING_TASK);

  [uiOwner_ insertListComponent:list_id componentSign:child_id];
};

void PaintingContextDarwinRef::RemoveListItemPaintingNode(int32_t list_id, int32_t child_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_REMOVE_LIST_PAINTING_TASK);

  [uiOwner_ removeListComponent:list_id componentSign:child_id];
}

void PaintingContextDarwinRef::SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id) {
  // For Darwin, we mock the paint_end timing by dispatching a task to the main queue.
  LynxPerformanceController* performanceController = perf_controller_;
  dispatch_async(dispatch_get_main_queue(), ^{
    [performanceController MarkPaintEndTimingIfNeeded];
  });
}

void PaintingContextDarwin::SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) {
  __weak LynxUIOwner* uiOwner = uiOwner_;
  Enqueue([uiOwner,
           keyframesDict = static_cast<PropBundleDarwin*>(keyframes_data.get())->dictionary()]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_SET_KEYFRAME_TASK);

    [uiOwner updateAnimationKeyframes:keyframesDict];
  });
}

void PaintingContextDarwin::SetUIOperationQueue(
    const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) {
  queue_ = queue;
}

void PaintingContextDarwin::SetInstanceId(const int32_t instance_id) {
  instance_id_ = instance_id;
  if (enable_create_ui_async_) {
    report::GlobalFeatureCounter::Count(report::LynxFeature::CPP_ENABLE_CREATE_UI_ASYNC,
                                        instance_id_);
  }
};

std::unique_ptr<pub::Value> PaintingContextDarwin::GetTextInfo(const std::string& content,
                                                               const pub::Value& info) {
  return TextUtilsDarwinHelper::GetTextInfo(content, info);
}

PaintingContextDarwin::PaintingContextDarwin(LynxUIOwner* owner, bool enable_create_ui_async)
    : uiOwner_(owner), enable_create_ui_async_(enable_create_ui_async) {
  platform_ref_ = std::make_shared<PaintingContextDarwinRef>(owner);
}

PaintingContextDarwin::~PaintingContextDarwin() {}

void PaintingContextDarwin::CreatePaintingNode(int sign, const std::string& tag,
                                               const fml::RefPtr<PropBundle>& painting_data,
                                               bool flatten, bool create_node_async,
                                               uint32_t node_index) {
  PropBundleDarwin* pda = static_cast<PropBundleDarwin*>(painting_data.get());
  NSString* tagName = [[NSString alloc] initWithUTF8String:tag.c_str()];
  // TODO(renzhongyue): Remove copy, we now own the shared_ptr of prop bundle here.
  NSDictionary* props = pda->dictionary();
  __weak LynxUIOwner* uiOwner = uiOwner_;

  // When enable_create_ui_async_, use createUIAsyncWithSign and createUISyncWithSign to create ui,
  // rather than use createUIWithSign. One enable_create_ui_async_ is verified to be stable, we will
  // remove createUIWithSign and default use createUIAsyncWithSign and createUISyncWithSign.
  if (enable_create_ui_async_) {
    TagSupportedState state;
    Class clazz = [uiOwner_ getTargetClass:tagName props:props supportedState:&state];

    if (create_node_async) {
      // Async create ui if the class is LynxUIView or LynxUIImage.
      std::promise<LynxUI*> promise;
      std::future<LynxUI*> future = promise.get_future();

      auto async_task = fml::MakeRefCounted<base::OnceTask<LynxUI*>>(
          [uiOwner, sign, tagName, clazz, state, eventSet = pda->event_set(),
           lepusEventSet = pda->lepus_event_set(), props, node_index,
           gestureDetectorSet = pda->gesture_detector_set(),
           promise = std::move(promise)]() mutable {
            @autoreleasepool {
              LynxUI* ui = [uiOwner createUIAsyncWithSign:sign
                                                  tagName:tagName
                                                    clazz:clazz
                                           supportedState:state
                                                 eventSet:eventSet
                                            lepusEventSet:lepusEventSet
                                                    props:props
                                                nodeIndex:node_index
                                       gestureDetectorSet:gestureDetectorSet];
              promise.set_value(ui);
            }
          },
          std::move(future));

      base::TaskRunnerManufactor::PostTaskToConcurrentLoop([async_task]() { async_task->Run(); },
                                                           base::ConcurrentTaskType::HIGH_PRIORITY);
      Enqueue([async_task, uiOwner, sign, tagName, props]() {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_CREATE_PAINTING_NODE_ASYNC);

        async_task->Run();
        LynxUI* ui = async_task->GetFuture().get();
        ui.view = [ui createView];
        [uiOwner processUIOnMainThread:ui withSign:sign tagName:tagName props:props];
      });
    } else {
      // Sync create ui if the class does not support async creating.
      Enqueue([uiOwner, sign, tagName, clazz, state, eventSet = pda->event_set(),
               lepusEventSet = pda->lepus_event_set(), props, node_index,
               gestureDetectorSet = pda->gesture_detector_set()]() {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_CREATE_PAINTING_NODE_SYNC);

        [uiOwner createUISyncWithSign:sign
                              tagName:tagName
                                clazz:clazz
                       supportedState:state
                             eventSet:eventSet
                        lepusEventSet:lepusEventSet
                                props:props
                            nodeIndex:node_index
                   gestureDetectorSet:gestureDetectorSet];
      });
    }
    return;
  }

  Enqueue([uiOwner, sign, tagName, eventSet = pda->event_set(),
           lepusEventSet = pda->lepus_event_set(), props, node_index,
           gestureDetectorSet = pda->gesture_detector_set()]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_CREATE_PAINTING_NODE);

    [uiOwner createUIWithSign:sign
                      tagName:tagName
                     eventSet:eventSet
                lepusEventSet:lepusEventSet
                        props:props
                    nodeIndex:node_index
           gestureDetectorSet:gestureDetectorSet];
  });
}

void PaintingContextDarwin::UpdatePaintingNode(int id, bool tend_to_flatten,
                                               const fml::RefPtr<PropBundle>& painting_data) {
  PropBundleDarwin* pda = static_cast<PropBundleDarwin*>(painting_data.get());
  __weak LynxUIOwner* uiOwner = uiOwner_;
  Enqueue([uiOwner, id, props = pda->dictionary(), eventSet = pda->event_set(),
           lepusEventSet = pda->lepus_event_set(),
           gestureDetectorSet = pda->gesture_detector_set()]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_PAINTING_NODE);

    [uiOwner updateUIWithSign:id
                        props:props
                     eventSet:eventSet
                lepusEventSet:lepusEventSet
           gestureDetectorSet:gestureDetectorSet];
  });
}

void PaintingContextDarwin::UpdateLayout(int sign, float x, float y, float width, float height,
                                         const float* paddings, const float* margins,
                                         const float* borders, const float* flatten_bounds,
                                         const float* sticky, float max_height,
                                         uint32_t node_index) {
  // top left right bottom for UIEdgeInset
#define UI_EDGE_INSETS(array) \
  array != nullptr ? UIEdgeInsetsMake(array[1], array[0], array[3], array[2]) : UIEdgeInsetsZero
  NSMutableArray* stickyArr;
  if (sticky != nil) {
    stickyArr = [[NSMutableArray alloc] init];
    for (int i = 0; i < 4; i++) {
      [stickyArr addObject:[NSNumber numberWithFloat:sticky[i]]];
    }
  }
  __weak LynxUIOwner* uiOwner = uiOwner_;
  Enqueue([uiOwner, sign, x, y, width, height, padding = UI_EDGE_INSETS(paddings),
           border = UI_EDGE_INSETS(borders), margin = UI_EDGE_INSETS(margins), stickyArr]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_LAYOUT_TASK);

    [uiOwner updateUI:sign
           layoutLeft:x
                  top:y
                width:width
               height:height
              padding:padding
               border:border
               margin:margin
               sticky:stickyArr];
  });
#undef UI_EDGE_INSETS
}

void PaintingContextDarwin::SetFrameAppBundle(int tag,
                                              const std::shared_ptr<LynxTemplateBundle>& bundle) {
  __weak LynxUIOwner* uiOwner = uiOwner_;
  auto platform_bundle = ConstructTemplateBundleFromNative(*bundle);
  Enqueue([uiOwner, tag, platform_bundle]() {
    [uiOwner setFrameAppBundle:platform_bundle withTag:tag];
  });
}

void PaintingContextDarwin::Flush() { queue_->Flush(); }

std::vector<float> PaintingContextDarwin::getBoundingClientOrigin(int id) {
  std::vector<float> res;
  LynxUI* ui = [uiOwner_ findUIBySign:id];
  if (ui != NULL) {
    CGRect re = [ui getBoundingClientRect];
    res.push_back(re.origin.x);
    res.push_back(re.origin.y);
  }
  return res;
}

std::vector<float> PaintingContextDarwin::getWindowSize(int id) {
  std::vector<float> res;
  CGSize size = UIScreen.mainScreen.bounds.size;
  res.push_back(size.width);
  res.push_back(size.height);
  return res;
}

lepus::Value PaintingContextDarwin::GetUITreeRecursive(LynxUI* ui) {
#if ENABLE_TESTBENCH_REPLAY
  CGRect re = [ui getBoundingClientRect];
  auto node_json = lepus::Dictionary::Create();

  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  BASE_STATIC_STRING_DECL(kLeft, "left");
  BASE_STATIC_STRING_DECL(kTop, "top");
  node_json->SetValue(kWidth, re.size.width);
  node_json->SetValue(kHeight, re.size.height);
  node_json->SetValue(kLeft, re.origin.x);
  node_json->SetValue(kTop, re.origin.y);

  // children
  auto children = lepus::CArray::Create();
  for (LynxUI* child in [ui.children reverseObjectEnumerator]) {
    children->emplace_back(GetUITreeRecursive(child));
  }
  BASE_STATIC_STRING_DECL(kChildren, "children");
  node_json->SetValue(kChildren, std::move(children));

  return lepus::Value(std::move(node_json));
#else
  return lepus::Value();
#endif
}

std::string PaintingContextDarwin::GetUITree() {
#if ENABLE_TESTBENCH_REPLAY
  LynxUI* root = (LynxUI*)[uiOwner_ rootUI];
  return lepus::lepusValueToJSONString(GetUITreeRecursive(root));
#else
  return "";
#endif
}

std::vector<float> PaintingContextDarwin::GetRectToWindow(int id) {
  std::vector<float> res;
  LynxUI* ui = [uiOwner_ findUIBySign:id];
  if (ui != NULL) {
    CGRect re = [ui getRectToWindow];
    int scale = UIScreen.mainScreen.scale;
    res.push_back(re.origin.x * scale);
    res.push_back(re.origin.y * scale);
    res.push_back(re.size.width * scale);
    res.push_back(re.size.height * scale);
  }
  return res;
}

std::vector<float> PaintingContextDarwin::GetRectToLynxView(int64_t id) {
  // x y width height
  std::vector<float> res;
  LynxUI* ui = [uiOwner_ findUIBySign:(int)id];
  if (ui != NULL) {
    CGRect re = [ui getBoundingClientRect];
    res.push_back(re.origin.x);
    res.push_back(re.origin.y);
    res.push_back(re.size.width);
    res.push_back(re.size.height);
  }
  return res;
}

std::vector<float> PaintingContextDarwin::ScrollBy(int64_t id, float width, float height) {
  auto runnable = ^{
    LynxUI* ui = [uiOwner_ findUIBySign:(int)id];
    CGPoint preOffset = ui.contentOffset;

    NSArray<NSNumber*>* res = [ui scrollBy:width deltaY:height];

    CGPoint postOffset = CGPointMake(res.firstObject.floatValue, res.lastObject.floatValue);

    float consumed_x = postOffset.x - preOffset.x;
    float consumed_y = postOffset.y - preOffset.y;
    float unconsumed_x = width - consumed_x;
    float unconsumed_y = height - consumed_y;
    return std::vector<float>{consumed_x, consumed_y, unconsumed_x, unconsumed_y};
  };

  if (dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
      dispatch_queue_get_label(dispatch_get_main_queue())) {
    return runnable();
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      runnable();
    });
    // align to default value in Android platform, It is not consumed when method return
    return std::vector<float>{0, 0, width, height};
  }
}

void PaintingContextDarwin::ConsumeGesture(int64_t idx, int32_t gesture_id,
                                           const pub::Value& params) {
  __weak LynxUIOwner* uiOwner = uiOwner_;
  auto lepusMap = pub::ValueUtils::ConvertValueToLepusValue(params);

  Enqueue([uiOwner, idx, gesture_id, map = std::move(lepusMap)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_LIST_REUSE_PAINTING_NODE);

    // Find the LynxUI associated with the given index (sign).
    LynxUI* ui = [uiOwner findUIBySign:(int)idx];

    // Check if the LynxUI exists and conforms to the LynxNewGestureDelegate protocol.
    // Consume gesture or not on the LynxUI. params: {inner: boolean, consume: boolean, ...}
    [ui consumeGesture:gesture_id params:convertLepusValueToNSObject(map)];
  });
}

/**
 * @param tag_name  tag name of the node to be queried
 * @return 32bit integer value representing data object related to tag name including layout node
 *     type and whether need to create node on a background thread.
 * Each data object value will adhere to the following layout:
 * Lower 16 bits represents layout node type
 * 17th bit represents whether node with tag name support async creation
 */
int32_t PaintingContextDarwin::GetTagInfo(const std::string& tag_name) {
  int32_t layout_node_type = static_cast<int32_t>(
      [uiOwner_ getTagInfo:[[NSString alloc] initWithUTF8String:tag_name.c_str()]]);
  bool is_virtual = layout_node_type & LayoutNodeType::VIRTUAL;
  if (is_virtual) {
    // For virtual nodes, we only need to return the layout node type.
    return (layout_node_type & 0xFFFF);
  }

  bool create_ui_async =
      (enable_create_ui_async_ &&
       [uiOwner_ needCreateUIAsync:[[NSString alloc] initWithUTF8String:tag_name.c_str()]] == YES);
  return ((create_ui_async ? 1 : 0) << 16 | (layout_node_type & 0xFFFF));
}

bool PaintingContextDarwin::IsFlatten(base::MoveOnlyClosure<bool, bool> func) {
  if (func != nullptr) {
    return func(false);
  }
  return false;
}

void PaintingContextDarwin::Invoke(
    int64_t element_id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  // needs to be passed by copying instead of passing a reference, because OC's block and C++'s
  // callback are not compatible.
  auto block = callback;
  __weak LynxUIOwner* owner = uiOwner_;
  auto lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);
  auto runnable = ^{
    auto callback = ^(int code, id _Nullable data) {
      // exec the following block on main thread.
      if (owner == nil) {
        return;
      }

      [owner.uiContext.lynxContext runOnTasmThread:^{
        // exec the block on tasm thread.
        block(code, PubLepusValue(LynxConvertToLepusValue(data)));
      }];
    };

    LynxUI* ui = [owner findUIBySign:(int)element_id];
    if (!ui) {
      NSString* msg =
          [NSString stringWithFormat:@"Worklet: node %lld does not have a LynxUI", element_id];
      callback(kUIMethodNoUiForNode, msg);
      return;
    }
    [LynxUIMethodProcessor invokeMethod:[[NSString alloc] initWithUTF8String:method.c_str()]
                             withParams:convertLepusValueToNSObject(lepus_params)
                             withResult:^(int code, id _Nullable data) {
                               if (dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                                   dispatch_queue_get_label(dispatch_get_main_queue())) {
                                 callback(code, data);
                               } else {
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                   callback(code, data);
                                 });
                               }
                             }
                                  forUI:ui];
  };
  if (dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
      dispatch_queue_get_label(dispatch_get_main_queue())) {
    runnable();
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      runnable();
    });
  }
}

void PaintingContextDarwin::OnFirstMeaningfulLayout() {}

void PaintingContextDarwin::UpdatePlatformExtraBundle(int32_t signature,
                                                      PlatformExtraBundle* bundle) {
  if (!bundle) {
    return;
  }

  auto platform_bundle = static_cast<PlatformExtraBundleDarwin*>(bundle);
  __weak LynxUIOwner* uiOwner = uiOwner_;
  Enqueue([uiOwner, signature, value = platform_bundle->PlatformBundle()]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_EXTRA_BUNDLE);

    [uiOwner onReceiveUIOperation:value onUI:signature];
  });
}

void PaintingContextDarwin::FinishLayoutOperation(const std::shared_ptr<PipelineOptions>& options) {
  is_layout_finish_ = true;
  __weak LynxUIOwner* uiOwner = uiOwner_;
  Enqueue([uiOwner, weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_), options]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_FINISH_LAYOUT_OPERATION);

    if (auto queue = weak_queue.lock()) {
      [uiOwner finishLayoutOperation:options->operation_id componentID:options->list_comp_id_];
      if (options->has_layout) {
        [uiOwner layoutDidFinish];
      }
      if (options->native_update_data_order_ == queue->GetNativeUpdateDataOrder()) {
        queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
      }
    }
  });
  if (options->native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

void PaintingContextDarwin::FinishTasmOperation(const std::shared_ptr<PipelineOptions>& options) {
  if (options->native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

bool PaintingContextDarwin::IsLayoutFinish() { return is_layout_finish_; }

void PaintingContextDarwin::ResetLayoutStatus() { is_layout_finish_ = false; }

// TODO(heshan):remove related invocation
void PaintingContextDarwin::LayoutDidFinish() {}

void PaintingContextDarwin::ForceFlush() { queue_->ForceFlush(); }

template <typename F>
void PaintingContextDarwin::Enqueue(F&& func) {
  queue_->EnqueueUIOperation([func = std::move(func)]() {
    @autoreleasepool {
      ExecuteSafely(func);
    }
  });
}

template <typename F>
void PaintingContextDarwin::EnqueueHighPriorityUIOperation(F&& func) {
  queue_->EnqueueHighPriorityUIOperation([func = std::move(func)]() {
    @autoreleasepool {
      ExecuteSafely(func);
    }
  });
}

shell::UIOperation PaintingContextDarwin::ExecuteOperationSafely(shell::UIOperation op) {
  return [func = std::move(op)]() {
    @autoreleasepool {
      ExecuteSafely(func);
    }
  };
}

LayoutResult PaintingContextDarwin::MeasureText(int sign, PropArray* array, int width,
                                                int width_mode, int height, int height_mode) {
  float measured_width = 0;   // esult[0];
  float measured_height = 0;  // result[1];
  float base_line = 0;        // result[2];

  return LayoutResult{measured_width, measured_height, base_line};
}

}  // namespace tasm
}  // namespace lynx
