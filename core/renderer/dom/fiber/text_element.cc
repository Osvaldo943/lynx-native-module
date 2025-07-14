// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <memory>
#include <utility>

#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {

TextElement::TextElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
  is_text_ = true;
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultTextOverflow() &&
                     !EnableLayoutInElementMode());
}

void TextElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultTextOverflow() &&
                     !EnableLayoutInElementMode());
}

void TextElement::SetStyleInternal(CSSPropertyID id,
                                   const tasm::CSSValue& value,
                                   bool force_update) {
  FiberElement::SetStyleInternal(id, value, force_update);

  if (id == kPropertyIDFontFamily) {
    if (!EnableLayoutInElementMode()) {
      EnqueueLayoutTask([this, value]() {
        ResolveAndFlushFontFaces(value.GetValue().String());
      });
    } else {
      ResolveAndFlushFontFaces(value.GetValue().String());
    }
  }
}

void TextElement::OnNodeAdded(FiberElement* child) {
  child->ConvertToInlineElement();
  UpdateRenderRootElementIfNecessary(child);
  if (!child->is_raw_text()) {
    has_inline_child_ = true;
  }
}

base::String TextElement::ConvertContent(const lepus::Value value) {
  auto result = value.String();
  if (result.empty()) {
    if (value.IsInt32()) {
      result = base::String(std::to_string(value.Int32()));
    } else if (value.IsInt64()) {
      result = base::String(std::to_string(value.Int64()));
    } else if (value.IsNumber()) {
      std::stringstream stream;
      stream << value.Number();
      result = stream.str();
    } else if (value.IsNaN()) {
      BASE_STATIC_STRING_DECL(kNaN, "NaN");
      result = kNaN;
    } else if (value.IsNil()) {
      BASE_STATIC_STRING_DECL(kNull, "null");
      result = kNull;
    } else if (value.IsUndefined()) {
      BASE_STATIC_STRING_DECL(kUndefined, "undefined");
      result = kUndefined;
    }
  }
  return result;
}

void TextElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  // sometimes, text-overflow is used as attribute, so we need to parse the
  // value as CSS style here. it's better to mark such kind of attribute as
  // internal attributes, which may be processed as const IDs

  BASE_STATIC_STRING_DECL(kTextAttr, "text");
  BASE_STATIC_STRING_DECL(kTextMaxlineAttr, "text-maxline");
  BASE_STATIC_STRING_DECL(kTextOverflowAttr, "text-overflow");

  if (EnableLayoutInElementMode()) {
    if (key.IsEqual(kTextAttr)) {
      content_ = ConvertContent(value);
      if (EnableLayoutInElementMode()) {
        content_utf16_length_ =
            GetUtf16SizeFromUtf8(content_.c_str(), content_.length());
      }
    } else if (key.IsEqual(kTextMaxlineAttr)) {
      EnsureTextProps();
      text_props_->text_max_line =
          value.IsNumber() ? value.Number() : std::stoi(value.StdString());
    } else {
      FiberElement::SetAttributeInternal(key, value);
    }
    return;
  }

  if (key.IsEqual(kTextOverflowAttr)) {
    CacheStyleFromAttributes(kPropertyIDTextOverflow, value);
    has_layout_only_props_ = false;
  } else if (key.IsEqual(kTextAttr) && !children().empty()) {
    // if setNativeProps with key "text" on TextElement, we need to update it's
    // children.
    if (children().begin()->get()->is_raw_text()) {
      RawTextElement* raw_text =
          static_cast<RawTextElement*>(children().begin()->get());
      raw_text->SetText(value);
    }
  } else {
    FiberElement::SetAttributeInternal(key, value);
  }
}

void TextElement::ConvertToInlineElement() {
  if (tag_.IsEqual(kElementXTextTag)) {
    tag_ = BASE_STATIC_STRING(kElementXInlineTextTag);
  } else {
    tag_ = BASE_STATIC_STRING(kElementInlineTextTag);
  }
  data_model()->set_tag(tag_);
  UpdateTagToLayoutBundle();
  FiberElement::ConvertToInlineElement();
}

void TextElement::ResolveAndFlushFontFaces(const base::String& font_family) {
  auto* fragment = GetRelatedCSSFragment();
  if (fragment && !fragment->GetFontFaceRuleMap().empty() &&
      !fragment->HasFontFacesResolved()) {
    // FIXME(linxs): parse the font face according to font_family, instead of
    // flushing all font faces
    SetFontFaces(fragment->GetFontFaceRuleMap());
    fragment->MarkFontFacesResolved(true);
  }
}

bool TextElement::ResolveStyleValue(CSSPropertyID id,
                                    const tasm::CSSValue& value,
                                    bool force_update) {
  bool has_processed = false;

  if (EnableLayoutInElementMode() && IsTextMeasurerWanted(id)) {
    if (computed_css_style()->SetValue(id, value)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = FiberElement::ResolveStyleValue(id, value, force_update);
  }

  return has_processed;
}

bool TextElement::ResetCSSValue(CSSPropertyID id) {
  bool has_processed = false;
  if (EnableLayoutInElementMode()) {
    if (id == kPropertyIDFontSize) {
      // font-size has been reset to default value in WillResetCSSValue
      return false;
    }
    if (computed_css_style()->ResetValue(id)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = FiberElement::ResetCSSValue(id);
  }

  return has_processed;
}

void TextElement::BuildTextPropsBuffer(std::string& output,
                                       size_t& current_length, bool use_utf16,
                                       PropArray* props) {
  size_t start = current_length;
  if (!content_.empty()) {
    output += content_.str();
    current_length += use_utf16 ? content_utf16_length_ : content_.length();
  }

  auto* child = first_render_child();
  while (child) {
    if (static_cast<FiberElement*>(child)->is_raw_text()) {
      auto* raw_text_child = static_cast<RawTextElement*>(child);
      const auto& raw_content = raw_text_child->content();
      if (!raw_content.empty()) {
        output += raw_content.str();
        current_length += use_utf16 ? raw_text_child->content_utf16_length()
                                    : raw_content.length();
      }
    } else if (child->is_text()) {
      // inline text
      static_cast<TextElement*>(child)->BuildTextPropsBuffer(
          output, current_length, use_utf16, props);
    } else if (child->is_image() || child->is_view()) {
      // inline image
      output += kInlinePlaceHolder;
      current_length += 1;  // placeholder's length is 1
      static_cast<FiberElement*>(child)->BuildAttributedStringProps(
          current_length - 1, current_length, props);
    }
    child = child->next_render_sibling();
  }

  auto end = current_length;
  if (end > start) {
    BuildAttributedStringProps(start, end, props);
  }
}

LayoutResult TextElement::Measure(float width, int32_t width_mode, float height,
                                  int32_t height_mode, bool final_measure) {
  if (is_inline_element()) {
    return LayoutResult(0, 0, 0);
  }
  auto props = element_manager_->GetPropBundleCreator()->CreatePropArray();
  if (!props) {
    LOGE("TextElement: no PropArray defined!")
    return LayoutResult(0, 0, 0);
  }
  std::string output_str;
  size_t current_length = 0;
  bool use_utf16 = is_inline_element_ || has_inline_child_;
  BuildTextPropsBuffer(output_str, current_length, use_utf16, props.get());

  props->AddProp(kPropTextString);
  props->AddProp(output_str.c_str());

  return element_manager_->MeasureText(id_, props.get(), width, width_mode,
                                       height, height_mode);
}

void TextElement::OnLayoutObjectCreated() {
  if (!is_inline_element()) {
    SetMeasureFunc(
        this, [](void* context, const starlight::Constraints& constraints,
                 bool final_measure) {
          TextElement* element = static_cast<TextElement*>(context);
          DCHECK(element);
          SLMeasureMode width_mode = constraints[starlight::kHorizontal].Mode();
          SLMeasureMode height_mode = constraints[starlight::kVertical].Mode();
          float width = IsSLIndefiniteMode(width_mode)
                            ? 0.f
                            : constraints[starlight::kHorizontal].Size();
          float height = IsSLIndefiniteMode(height_mode)
                             ? 0.f
                             : constraints[starlight::kVertical].Size();

          LayoutResult result = element->Measure(width, width_mode, height,
                                                 height_mode, final_measure);

          return FloatSize(result.width_, result.height_, result.baseline_);
        });
  }
}

void TextElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                           double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    property_bits_.Set(kPropertyIDFontSize);
  } else {
    FiberElement::UpdateLayoutNodeFontSize(cur_node_font_size,
                                           root_node_font_size);
  }
}

// static
void TextElement::BuildAttributedStringProps(size_t pos_start, size_t pos_end,
                                             PropArray* props) {
  if (!text_props_ && !property_bits_.HasAny()) {
    return;
  }
  // only inline text need the pass the range，   kPropRangeStart should be
  // the first key
  if (is_inline_element()) {
    props->AddProp(kPropInlineStart);
    props->AddProp(static_cast<int>(pos_start));
  }

  // styles
  const auto& text_attributes = computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value()) {
    for (CSSPropertyID id : property_bits_) {
      switch (id) {
        case kPropertyIDFontSize:
          props->AddProp(kTextPropFontSize);
          props->AddProp(
              static_cast<float>(computed_css_style()->GetFontSize()));
          break;

        case kPropertyIDColor:
          props->AddProp(kTextPropColor);
          props->AddProp(static_cast<int>(text_attributes->color));
          // FIXME(linxs): use another key to indicate color gradient
          break;

        case kPropertyIDWhiteSpace:
          props->AddProp(kTextPropWhiteSpace);
          props->AddProp(static_cast<int>(text_attributes->white_space));
          break;

        case kPropertyIDTextOverflow:
          props->AddProp(kTextPropTextOverflow);
          props->AddProp(static_cast<int>(text_attributes->text_overflow));
          break;

        case kPropertyIDFontWeight:
          props->AddProp(kTextPropFontWeight);
          props->AddProp(static_cast<int>(text_attributes->font_weight));
          break;
        case kPropertyIDFontStyle:
          props->AddProp(kTextPropFontStyle);
          props->AddProp(static_cast<int>(text_attributes->font_style));
          break;

        case kPropertyIDFontFamily:
          props->AddProp(kTextPropFontFamily);
          props->AddProp(text_attributes->font_family.c_str());
          break;

        case kPropertyIDLineHeight:
          props->AddProp(kTextPropLineHeight);
          props->AddProp(text_attributes->computed_line_height);
          break;

        case kPropertyIDLetterSpacing:
          props->AddProp(kTextPropLetterSpacing);
          props->AddProp(text_attributes->letter_spacing);
          break;

        case kPropertyIDTextAlign:
          props->AddProp(kTextPropTextAlign);
          props->AddProp(static_cast<int>(text_attributes->text_align));
          break;

        case kPropertyIDVerticalAlign:
          props->AddProp(kTextPropVerticalAlign);
          props->AddProp(static_cast<int>(text_attributes->vertical_align));
          props->AddProp(text_attributes->vertical_align_length);
          break;

        default:
          break;
      }
    }
  }

  // attributes
  // text_maxline
  if (text_props_) {
    if (text_props_->text_max_line) {
      props->AddProp(kTextPropTextMaxLine);
      props->AddProp(*text_props_->text_max_line);
    }
  }

  // only inline text need the pass the range, kPropRangeEnd should be the
  // first key
  if (is_inline_element()) {
    props->AddProp(kPropInlineEnd);
    props->AddProp(static_cast<int>(pos_end));
  }
}

}  // namespace tasm
}  // namespace lynx
