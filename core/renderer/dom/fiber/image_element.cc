// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/image_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

ImageElement::ImageElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {}

void ImageElement::OnNodeAdded(FiberElement* child) {
  LOGE("image element can not insert any child!!!");
}

bool ImageElement::DisableFlattenWithOpacity() { return false; }

void ImageElement::ConvertToInlineElement() {
  if (tag_.IsEqual(kElementXImageTag)) {
    tag_ = BASE_STATIC_STRING(kElementXInlineImageTag);
  } else {
    tag_ = BASE_STATIC_STRING(kElementInlineImageTag);
  }
  data_model()->set_tag(tag_);
  UpdateTagToLayoutBundle();
  FiberElement::ConvertToInlineElement();
}

void ImageElement::SetAttributeInternal(const base::String& key,
                                        const lepus::Value& value) {
  // TODO(songshourui.null): we can process image's attribute in C++ to optimize
  // the performance.
  if (EnableLayoutInElementMode()) {
    attr_map_[key] = value;
  }
  FiberElement::SetAttributeInternal(key, value);
}

void ImageElement::BuildAttributedStringProps(size_t start, size_t end,
                                              PropArray* props) {
  // inline range start
  props->AddProp(kPropInlineStart);
  props->AddProp(static_cast<int>(start));

  // src
  props->AddProp(kPropImageSrc);
  props->AddProp(attr_map_[kSrc].CString());

  // mode
  // TBD

  // size
  props->AddProp(kPropRectSize);
  float width =
      starlight::NLengthToFakeLayoutUnit(slnode()->GetCSSStyle()->GetWidth())
          .ClampIndefiniteToZero()
          .ToFloat();
  float height =
      starlight::NLengthToFakeLayoutUnit(slnode()->GetCSSStyle()->GetHeight())
          .ClampIndefiniteToZero()
          .ToFloat();
  props->AddProp(static_cast<int>(width));
  props->AddProp(static_cast<int>(height));

  // margin
  int margin_left =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginLeft())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_top =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginRight())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_right =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginTop())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_bottom =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginBottom())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  if (margin_left | margin_top | margin_right | margin_bottom) {
    props->AddProp(kPropMargin);
    props->AddProp(margin_left);
    props->AddProp(margin_top);
    props->AddProp(margin_right);
    props->AddProp(margin_bottom);
  }

  if (computed_css_style()->HasBorderRadius()) {
    props->AddProp(kPropBorderRadius);
    // only support: left,top,right,bottom border-radius for such mode
    const auto& top_left = computed_css_style()->GetSimpleBorderTopLeftRadius();
    const auto& top_right =
        computed_css_style()->GetSimpleBorderTopLeftRadius();
    const auto& bottom_left =
        computed_css_style()->GetSimpleBorderTopLeftRadius();
    const auto& bottom_right =
        computed_css_style()->GetSimpleBorderTopLeftRadius();
    const auto resolve_length = [](PropArray* props,
                                   const starlight::NLength& length) {
      if (length.NumericLength().ContainsPercentage()) {
        props->AddProp(static_cast<float>(
            length.NumericLength().GetPercentagePart() / 100.f));
        props->AddProp(
            static_cast<int>(starlight::PlatformLengthUnit::PERCENTAGE));
      }
      if (length.NumericLength().ContainsFixedValue() ||
          !length.NumericLength().ContainsPercentage()) {
        props->AddProp(
            static_cast<float>(length.NumericLength().GetFixedPart()));
        props->AddProp(static_cast<int>(starlight::PlatformLengthUnit::NUMBER));
      }
    };
    resolve_length(props, top_left);
    resolve_length(props, top_right);
    resolve_length(props, bottom_left);
    resolve_length(props, bottom_right);
  }

  // inline range end
  props->AddProp(kPropInlineEnd);
  props->AddProp(static_cast<int>(end));
}

}  // namespace tasm
}  // namespace lynx
