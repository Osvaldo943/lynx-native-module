// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_element_observer_impl.h"

#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/tracing/devtool_trace_event_def.h"

namespace lynx {
namespace devtool {

InspectorElementObserverImpl::InspectorElementObserverImpl(
    const std::shared_ptr<InspectorTasmExecutor> &element_executor)
    : element_executor_wp_(element_executor) {}

void InspectorElementObserverImpl::OnDocumentUpdated() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_DOCUMENT_UPDATE);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnDocumentUpdated();
}

void InspectorElementObserverImpl::OnElementNodeAdded(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_ELEMENT_NODE_ADDED);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementNodeAdded(ptr);
}

void InspectorElementObserverImpl::OnElementNodeRemoved(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_ELEMENT_NODE_REMOVED);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementNodeRemoved(ptr);
}

void InspectorElementObserverImpl::OnCharacterDataModified(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_CHARACTER_DATA_MODIFIED);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnCharacterDataModified(ptr);
}

void InspectorElementObserverImpl::OnElementDataModelSet(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_ELEMENT_DATA_MODEL_SET);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementDataModelSet(ptr);
}

void InspectorElementObserverImpl::OnElementManagerWillDestroy() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              ELEMENT_OBSERVER_ON_ELEMENT_MANAGER_WILL_DESTORY);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementManagerWillDestroy();
}

void InspectorElementObserverImpl::OnSetNativeProps(lynx::tasm::Element *ptr,
                                                    const std::string &name,
                                                    const std::string &value,
                                                    bool is_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_SET_NATIVE_PROPS);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnSetNativeProps(ptr, name, value, is_style);
}

void InspectorElementObserverImpl::OnCSSStyleSheetAdded(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_OBSERVER_ON_CSS_STYLE_SHEET_ADDED);
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnCSSStyleSheetAdded(ptr);
}

void InspectorElementObserverImpl::OnComponentUselessUpdate(
    const std::string &component_name, const lynx::lepus::Value &properties) {}

std::map<lynx::devtool::DevToolFunction, std::function<void(const base::any &)>>
InspectorElementObserverImpl::GetDevToolFunction() {
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_executor, "element_executor is null",
                                  {});
  return element_executor->GetFunctionForElementMap();
}

}  // namespace devtool
}  // namespace lynx
