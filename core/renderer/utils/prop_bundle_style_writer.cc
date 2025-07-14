// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/prop_bundle_style_writer.h"

#include <cstdint>
#include <vector>

#include "base/include/algorithm.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

// This class is used to push different values from style module. Now, all
// properties are lepus::Value got from ComputedCSSValue. So we use a
// DefaultWriterFunc to handle it. We are going to make it cleaner by
// refactoring values to property specified values (e.g. ColorValue,
// GradientValue, ImageValue, etc.). Once a specified value is ready, we'll add
// corresponding writer here.

void PropBundleStyleWriter::PushStyleToBundle(
    PropBundle* bundle, CSSPropertyID id, starlight::ComputedCSSStyle* style,
    bool use_specific_writer) {
  if (id > kPropertyStart && id < kPropertyEnd) {
    if (void (*const writer)(PropBundle*, starlight::ComputedCSSStyle*) =
            GetWriter()[id]) {
      if (use_specific_writer) {
        (*writer)(bundle, style);
      } else {
        DefaultWriterFunc(bundle, id, style);
      }
      return;
    }
  }
  LynxWarning(
      false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
      "PropBundleStyleWriter can't find writer function for style id:%d.", id);
}

#define FOREACH_NEED_OPT_PROPERTY(V)     \
  V(BackgroundImage)                     \
  V(BackgroundPosition)                  \
  V(BackgroundSize)                      \
  V(ClipPath)                            \
  V(Cursor)                              \
  V(MaskImage)                           \
  V(MaskSize)                            \
  V(MaskPosition)                        \
  V(Filter)                              \
  V(Transform)                           \
  V(TransformOrigin)                     \
  V(Animation)                           \
  V(AnimationName)                       \
  V(AnimationTimingFunction)             \
  V(LayoutAnimationCreateTimingFunction) \
  V(LayoutAnimationDeleteTimingFunction) \
  V(LayoutAnimationUpdateTimingFunction) \
  V(Transition)                          \
  V(EnterTransitionName)                 \
  V(ExitTransitionName)                  \
  V(PauseTransitionName)                 \
  V(ResumeTransitionName)                \
  V(BoxShadow)                           \
  V(TextDecoration)                      \
  V(TextShadow)                          \
  V(VerticalAlign)                       \
  V(BorderRadius)                        \
  V(BorderTopLeftRadius)                 \
  V(BorderTopRightRadius)                \
  V(BorderBottomRightRadius)             \
  V(BorderBottomLeftRadius)              \
  V(Perspective)                         \
  V(TextIndent)                          \
  V(XAutoFontSize)                       \
  V(XAutoFontSizePresetSizes)

#define NAME_TO_LEPUS(name) name##ToLepus
#define WRITE_STYLE_IMPL(name)                                               \
  void PropBundleStyleWriter::Write##name(                                   \
      PropBundle* bundle, starlight::ComputedCSSStyle* style) {              \
    bundle->SetPropsByID(CSSPropertyID::kPropertyID##name,                   \
                         pub::ValueImplLepus(style->NAME_TO_LEPUS(name)())); \
  }
FOREACH_NEED_OPT_PROPERTY(WRITE_STYLE_IMPL)
#undef WRITE_STYLE_IMPL
#undef NAME_TO_LEPUS

// TODO(songshourui.null): the following methods should directly call specific
// get methods of ComputedCSSStyle to obtain the value of CSSPropertyID, pushing
// it directly to PropBundle, instead of calling DefaultWriterFunc. This will
// optimize performance.
void PropBundleStyleWriter::WriteOpacity(PropBundle* bundle,
                                         starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOpacity, style->GetOpacity());
}

void PropBundleStyleWriter::WritePosition(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDPosition,
                       static_cast<uint32_t>(style->GetPosition()));
}

void PropBundleStyleWriter::WriteOverflow(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOverflow,
                       static_cast<uint32_t>(style->GetOverflow()));
}

void PropBundleStyleWriter::WriteOverflowX(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOverflowX,
                       static_cast<uint32_t>(style->GetOverflowX()));
}

void PropBundleStyleWriter::WriteOverflowY(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOverflowY,
                       static_cast<uint32_t>(style->GetOverflowY()));
}

void PropBundleStyleWriter::WriteFontSize(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  auto& text_attr = style->GetTextAttributes();
  if (text_attr.has_value()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDFontSize,
                         text_attr->font_size);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDFontSize);
  }
}

void PropBundleStyleWriter::WriteLineHeight(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attr = style->GetTextAttributes();
  if (text_attr.has_value()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDLineHeight,
                         text_attr->computed_line_height);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDLineHeight);
  }
}

void PropBundleStyleWriter::WriteLetterSpacing(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attr = style->GetTextAttributes();
  if (text_attr.has_value()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDLetterSpacing,
                         text_attr->letter_spacing);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDLetterSpacing);
  }
}

void PropBundleStyleWriter::WriteLineSpacing(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attr = style->GetTextAttributes();
  if (text_attr.has_value()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDLineSpacing,
                         text_attr->line_spacing);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDLineSpacing);
  }
}

void PropBundleStyleWriter::WriteColor(PropBundle* bundle,
                                       starlight::ComputedCSSStyle* style) {
  auto& text_attr = style->GetTextAttributes();
  if (text_attr.has_value()) {
    if (text_attr->text_gradient.has_value() &&
        text_attr->text_gradient->IsArray()) {
      bundle->SetPropsByID(CSSPropertyID::kPropertyIDColor,
                           pub::ValueImplLepus(*text_attr->text_gradient));
    } else {
      bundle->SetPropsByID(CSSPropertyID::kPropertyIDColor, text_attr->color);
    }
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDColor);
  }
}

void PropBundleStyleWriter::WriteBackground(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteBackground should not be executed for now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBackground);
}

void PropBundleStyleWriter::WriteBackgroundClip(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetBackgroundData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundClip,
                         data->image_data->clip);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundClip,
                         std::vector<uint32_t>());
  }
}

void PropBundleStyleWriter::WriteBackgroundColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetBackgroundData();
  if (data.has_value()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundColor,
                         data->color);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBackgroundColor);
  }
}

void PropBundleStyleWriter::WriteBackgroundOrigin(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetBackgroundData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundOrigin,
                         data->image_data->origin);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundOrigin,
                         std::vector<uint8_t>());
  }
}

void PropBundleStyleWriter::WriteBackgroundRepeat(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetBackgroundData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundRepeat,
                         data->image_data->repeat);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDBackgroundRepeat,
                         std::vector<uint8_t>());
  }
}

void PropBundleStyleWriter::WriteMaskOrigin(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetMaskData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskOrigin,
                         data->image_data->origin);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskOrigin,
                         std::vector<uint8_t>());
  }
}

void PropBundleStyleWriter::WriteMaskClip(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  auto& data = style->GetMaskData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskClip,
                         data->image_data->clip);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskClip,
                         std::vector<uint8_t>());
  }
}

void PropBundleStyleWriter::WriteMaskRepeat(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& data = style->GetMaskData();
  if (data && data->image_data) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskRepeat,
                         data->image_data->repeat);
  } else {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDMaskRepeat,
                         std::vector<uint8_t>());
  }
}

void PropBundleStyleWriter::WriteBorderLeftColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderLeftColor,
        layout_computed_style->surround_data_.border_data_->color_left);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderLeftColor);
  }
}

void PropBundleStyleWriter::WriteBorderRightColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderRightColor,
        layout_computed_style->surround_data_.border_data_->color_right);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderRightColor);
  }
}

void PropBundleStyleWriter::WriteBorderTopColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderTopColor,
        layout_computed_style->surround_data_.border_data_->color_top);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderTopColor);
  }
}

void PropBundleStyleWriter::WriteBorderBottomColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderBottomColor,
        layout_computed_style->surround_data_.border_data_->color_bottom);
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderBottomColor);
  }
}

void PropBundleStyleWriter::WriteBorderLeftWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDBorderLeftWidth,
      static_cast<double>(layout_computed_style->GetBorderLeftWidth()));
}

void PropBundleStyleWriter::WriteBorderRightWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDBorderRightWidth,
      static_cast<double>(layout_computed_style->GetBorderRightWidth()));
}

void PropBundleStyleWriter::WriteBorderTopWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDBorderTopWidth,
      static_cast<double>(layout_computed_style->GetBorderTopWidth()));
}

void PropBundleStyleWriter::WriteBorderBottomWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDBorderBottomWidth,
      static_cast<double>(layout_computed_style->GetBorderBottomWidth()));
}

void PropBundleStyleWriter::WriteAnimationDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationDuration,
                       static_cast<double>(animation_data->front().duration));
}

void PropBundleStyleWriter::WriteAnimationDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationDelay,
                       static_cast<double>(animation_data->front().delay));
}

void PropBundleStyleWriter::WriteAnimationIterationCount(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationIterationCount,
                       animation_data->front().iteration_count);
}

void PropBundleStyleWriter::WriteAnimationDirection(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationDirection,
                       static_cast<int>(animation_data->front().direction));
}

void PropBundleStyleWriter::WriteAnimationFillMode(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationFillMode,
                       static_cast<int>(animation_data->front().fill_mode));
}

void PropBundleStyleWriter::WriteAnimationPlayState(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& animation_data = style->GetAnimationData();
  DCHECK(animation_data && !animation_data->empty());
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDAnimationPlayState,
                       static_cast<int>(animation_data->front().play_state));
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationCreateDuration,
      static_cast<double>(layout_animation_data->create_ani.duration));
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationCreateDelay,
      static_cast<double>(layout_animation_data->create_ani.delay));
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationCreateProperty,
      static_cast<int>(layout_animation_data->create_ani.property));
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationDeleteDuration,
      static_cast<double>(layout_animation_data->delete_ani.duration));
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationDeleteDelay,
      static_cast<double>(layout_animation_data->delete_ani.delay));
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationDeleteProperty,
      static_cast<int>(layout_animation_data->delete_ani.property));
}

void PropBundleStyleWriter::WriteLayoutAnimationUpdateDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationUpdateDuration,
      static_cast<double>(layout_animation_data->update_ani.duration));
}

void PropBundleStyleWriter::WriteLayoutAnimationUpdateDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& layout_animation_data = style->GetLayoutAnimationData();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDLayoutAnimationUpdateDelay,
      static_cast<double>(layout_animation_data->update_ani.delay));
}

void PropBundleStyleWriter::WriteTransitionProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteTransitionProperty should not be executed "
      "for now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTransitionProperty);
}

void PropBundleStyleWriter::WriteTransitionDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteTransitionDuration should not be executed "
      "for now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTransitionDuration);
}

void PropBundleStyleWriter::WriteTransitionDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteTransitionDelay should not be executed for "
      "now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTransitionDelay);
}

void PropBundleStyleWriter::WriteTransitionTimingFunction(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteTransitionTimingFunction should not be "
      "executed for now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTransitionTimingFunction);
}

void PropBundleStyleWriter::WriteVisibility(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDVisibility,
                       static_cast<int>(style->GetVisibilityData()));
}

void PropBundleStyleWriter::WriteBorderLeftStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderLeftStyle,
        static_cast<int>(
            layout_computed_style->surround_data_.border_data_->style_left));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderLeftStyle);
  }
}

void PropBundleStyleWriter::WriteBorderRightStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderRightStyle,
        static_cast<int>(
            layout_computed_style->surround_data_.border_data_->style_right));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderRightStyle);
  }
}

void PropBundleStyleWriter::WriteBorderTopStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderTopStyle,
        static_cast<int>(
            layout_computed_style->surround_data_.border_data_->style_top));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderTopStyle);
  }
}

void PropBundleStyleWriter::WriteBorderBottomStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();

  if (layout_computed_style->surround_data_.border_data_) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDBorderBottomStyle,
        static_cast<int>(
            layout_computed_style->surround_data_.border_data_->style_bottom));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderBottomStyle);
  }
}

void PropBundleStyleWriter::WriteOutlineColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOutlineColor,
                       style->GetOutLineData()->color);
}

void PropBundleStyleWriter::WriteOutlineStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOutlineStyle,
                       static_cast<uint32_t>(style->GetOutLineData()->style));
}

void PropBundleStyleWriter::WriteOutlineWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOutlineWidth,
                       static_cast<double>(style->GetOutLineData()->width));
}

void PropBundleStyleWriter::WriteBorderColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteBorderColor should not be executed for "
      "now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDBorderColor);
}

void PropBundleStyleWriter::WriteFontFamily(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDFontFamily,
                         text_attributes->font_family.c_str());
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDFontFamily);
  }
}

void PropBundleStyleWriter::WriteCaretColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDCaretColor,
                       style->GetCaretColor().c_str());
}

void PropBundleStyleWriter::WriteDirection(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  auto layout_computed_style = style->GetLayoutComputedStyle();
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDDirection,
      static_cast<int>(layout_computed_style->direction_ ==
                               starlight::DirectionType::kLynxRtl
                           ? starlight::DirectionType::kRtl
                           : layout_computed_style->direction_));
}

void PropBundleStyleWriter::WriteWhiteSpace(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();
  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDWhiteSpace,
                         static_cast<int>(text_attributes->white_space));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDWhiteSpace);
  }
}

void PropBundleStyleWriter::WriteFontWeight(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDFontWeight,
                         static_cast<int>(text_attributes->font_weight));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDFontWeight);
  }
}

void PropBundleStyleWriter::WriteWordBreak(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDWordBreak,
                         static_cast<int>(text_attributes->word_break));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDWordBreak);
  }
}

void PropBundleStyleWriter::WriteFontStyle(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDFontStyle,
                         static_cast<int>(text_attributes->font_style));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDFontStyle);
  }
}

void PropBundleStyleWriter::WriteTextAlign(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDTextAlign,
                         static_cast<int>(text_attributes->text_align));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextAlign);
  }
}

void PropBundleStyleWriter::WriteTextOverflow(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();
  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDTextOverflow,
                         static_cast<int>(text_attributes->text_overflow));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextOverflow);
  }
}

void PropBundleStyleWriter::WriteTextDecorationColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDTextDecorationColor,
                         text_attributes->decoration_color);

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextDecorationColor);
  }
}

void PropBundleStyleWriter::WriteZIndex(PropBundle* bundle,
                                        starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDZIndex, style->GetZIndex());
}

void PropBundleStyleWriter::WriteImageRendering(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDImageRendering,
                       static_cast<uint32_t>(style->GetImageRendering()));
}

void PropBundleStyleWriter::WriteListMainAxisGap(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDListMainAxisGap,
      static_cast<double>(
          style->GetLayoutComputedStyle()->GetListMainAxisGap()));
}

void PropBundleStyleWriter::WriteListCrossAxisGap(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(
      CSSPropertyID::kPropertyIDListCrossAxisGap,
      static_cast<double>(
          style->GetLayoutComputedStyle()->GetListCrossAxisGap()));
}

void PropBundleStyleWriter::WriteOffsetPath(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto path = style->GetOffsetPath();
  if (path.get()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetPath,
                         pub::ValueImplLepus(lepus::Value(path)));
  } else {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDOffsetPath,
        pub::ValueImplLepus(lepus::Value(lepus::CArray::Create())));
  }
}

void PropBundleStyleWriter::WriteOffsetDistance(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetDistance,
                       style->GetOffsetDistance());
}

void PropBundleStyleWriter::WriteOffsetRotate(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetRotate,
                       style->GetOffsetRotate());
}

void PropBundleStyleWriter::WriteTextStroke(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  LOGE(
      "PropBundleStyleWriter::WriteTextStroke should not be executed for now.");
  bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextStroke);
}

void PropBundleStyleWriter::WriteTextStrokeWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();

  if (text_attributes) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDTextStrokeWidth,
        static_cast<double>(text_attributes->text_stroke_width));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextStrokeWidth);
  }
}

void PropBundleStyleWriter::WriteTextStrokeColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();
  if (text_attributes) {
    bundle->SetPropsByID(
        CSSPropertyID::kPropertyIDTextStrokeColor,
        static_cast<uint32_t>(text_attributes->text_stroke_color));

  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDTextStrokeColor);
  }
}

void PropBundleStyleWriter::WriteHyphens(PropBundle* bundle,
                                         starlight::ComputedCSSStyle* style) {
  auto& text_attributes = style->GetTextAttributes();
  if (text_attributes) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDHyphens,
                         static_cast<uint32_t>(text_attributes->hyphens));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDHyphens);
  }
}

void PropBundleStyleWriter::WriteXAppRegion(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDXAppRegion,
                       static_cast<uint32_t>(style->GetAppRegion()));
}

void PropBundleStyleWriter::WriteXHandleSize(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDXHandleSize,
                       static_cast<double>(style->GetHandleSize()));
}

void PropBundleStyleWriter::WriteXHandleColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDXHandleColor,
                       style->GetHandleColor());
}

const std::array<PropBundleStyleWriter::WriterFunc, kPropertyEnd>&
PropBundleStyleWriter::GetWriter() {
  static constexpr std::array<WriterFunc, kPropertyEnd> kSpecificWriter = [] {
    std::array<WriterFunc, kPropertyEnd> writer = {nullptr};
#define SET_STYLE_WRITER(name) writer[kPropertyID##name] = &Write##name;
    FOREACH_PLATFORM_PROPERTY(SET_STYLE_WRITER);
#undef SET_STYLE_WRITER
    return writer;
  }();

  return kSpecificWriter;
}

void PropBundleStyleWriter::DefaultWriterFunc(
    PropBundle* bundle, CSSPropertyID id, starlight::ComputedCSSStyle* style) {
  switch (const lepus_value style_value = style->GetValue(id);
          style_value.Type()) {
    case lepus::Value_Int32:
    case lepus::Value_Int64:
      bundle->SetPropsByID(id, static_cast<int>(style_value.Number()));
      break;
    case lepus::Value_UInt32:
    case lepus::Value_UInt64:
      bundle->SetPropsByID(id, static_cast<unsigned int>(style_value.Number()));
      break;
    case lepus::Value_Double:
      bundle->SetPropsByID(id, style_value.Number());
      break;
    case lepus::Value_Bool:
      bundle->SetPropsByID(id, style_value.Bool());
      break;
    case lepus::Value_String:
      bundle->SetPropsByID(id, style_value.CString());
      break;
    case lepus::Value_Array:
    case lepus::Value_Table:
      bundle->SetPropsByID(id, pub::ValueImplLepus(style_value));
      break;
    case lepus::Value_Nil:
      bundle->SetNullPropsByID(id);
      break;
    default:
      LynxWarning(false, error::E_CSS, "ResolveStyleValue");
      break;
  }
}
}  // namespace tasm
}  // namespace lynx
