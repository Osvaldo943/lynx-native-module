// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/view_element.h"

#include <memory>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

ViewElement::ViewElement(ElementManager* manager)
    : FiberElement(manager, BASE_STATIC_STRING(kElementViewTag)) {
  MarkCanBeLayoutOnly(true);
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultOverflowVisible());
}

void ViewElement::ConvertToInlineElement() { MarkAsInline(); }

void ViewElement::OnNodeAdded(FiberElement* child) {
  UpdateRenderRootElementIfNecessary(child);
}

void ViewElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(element_manager_->GetDefaultOverflowVisible());
}

void ViewElement::BuildAttributedStringProps(size_t start, size_t end,
                                             PropArray* props) {
  // range start
  props->AddProp(kPropInlineStart);
  props->AddProp(static_cast<int>(start));

  props->AddProp(kPropInlineView);

  // TODO(linxs): width, height...
  // range end
  props->AddProp(kPropInlineEnd);
  props->AddProp(static_cast<int>(end));
}

}  // namespace tasm
}  // namespace lynx
