// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/utils/prop_bundle_style_writer_unittest.h"

#include "core/renderer/css/computed_css_style.h"

namespace lynx {
namespace tasm {
namespace test {

TEST_F(PropBundleStyleWriterTest, TestWriterImpl) {
  PropBundleMock bundle;
  starlight::ComputedCSSStyle style(1, 1);

  style.animation_data_.emplace();
  style.animation_data_->push_back(starlight::AnimationData());

  style.transform_origin_.emplace();
  style.layout_animation_data_.emplace();
  style.enter_transition_data_.emplace();
  style.exit_transition_data_.emplace();
  style.pause_transition_data_.emplace();
  style.resume_transition_data_.emplace();
  style.background_data_.emplace();
  style.mask_data_.emplace();
  style.layout_animation_data_.emplace();
  style.outline_.emplace();
  style.transform_raw_.emplace();
  style.transition_data_.emplace();
  style.box_shadow_.emplace();
  style.text_attributes_.emplace(0);
  style.transform_origin_.emplace();
  style.filter_.emplace();
  style.perspective_data_.emplace();
  style.cursor_.emplace();

#define CALL_STYLE_WRITER(name) \
  PropBundleStyleWriter::Write##name(&bundle, &style);
  FOREACH_PLATFORM_PROPERTY(CALL_STYLE_WRITER);
#undef CALL_STYLE_WRITER
}

#define TEST_SPECIFIC_STYLE_WRITER(name)                         \
  PropBundleStyleWriter::Write##name(&new_bundle_, &style_);     \
  PropBundleStyleWriter::DefaultWriterFunc(                      \
      &old_bundle_, CSSPropertyID::kPropertyID##name, &style_);  \
  EXPECT_EQ(new_bundle_.props_[CSSProperty::GetPropertyNameCStr( \
                CSSPropertyID::kPropertyID##name)],              \
            old_bundle_.props_[CSSProperty::GetPropertyNameCStr( \
                CSSPropertyID::kPropertyID##name)]);

TEST_F(PropBundleStyleWriterTest, TestWriteOpacity) {
  style_.opacity_ = 1.0;
  TEST_SPECIFIC_STYLE_WRITER(Opacity);

  style_.opacity_ = 0;
  TEST_SPECIFIC_STYLE_WRITER(Opacity);

  style_.opacity_ = 2;
  TEST_SPECIFIC_STYLE_WRITER(Opacity);

  EXPECT_EQ(new_bundle_.props_["opacity"], lepus::Value(2.0f));
}

TEST_F(PropBundleStyleWriterTest, TestStyleWriter) {
  style_.animation_data_.emplace();
  style_.animation_data_->push_back(starlight::AnimationData());

  style_.transform_origin_.emplace();
  style_.layout_animation_data_.emplace();
  style_.enter_transition_data_.emplace();
  style_.exit_transition_data_.emplace();
  style_.pause_transition_data_.emplace();
  style_.resume_transition_data_.emplace();
  style_.background_data_.emplace();
  style_.mask_data_.emplace();
  style_.layout_animation_data_.emplace();
  style_.outline_.emplace();
  style_.transform_raw_.emplace();
  style_.transition_data_.emplace();
  style_.box_shadow_.emplace();
  style_.text_attributes_.emplace(0);
  style_.transform_origin_.emplace();
  style_.filter_.emplace();
  style_.perspective_data_.emplace();
  style_.cursor_.emplace();

  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.position_ = starlight::PositionType::kAbsolute;
  TEST_SPECIFIC_STYLE_WRITER(Position);

  style_.layout_computed_style_.position_ = starlight::PositionType::kRelative;
  TEST_SPECIFIC_STYLE_WRITER(Position);

  style_.layout_computed_style_.position_ = starlight::PositionType::kFixed;
  TEST_SPECIFIC_STYLE_WRITER(Position);

  style_.layout_computed_style_.position_ = starlight::PositionType::kSticky;
  TEST_SPECIFIC_STYLE_WRITER(Position);

  style_.overflow_ = starlight::OverflowType::kVisible;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_ = starlight::OverflowType::kHidden;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_ = starlight::OverflowType::kScroll;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_x_ = starlight::OverflowType::kVisible;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_x_ = starlight::OverflowType::kHidden;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_x_ = starlight::OverflowType::kScroll;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_y_ = starlight::OverflowType::kVisible;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_y_ = starlight::OverflowType::kHidden;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.overflow_y_ = starlight::OverflowType::kScroll;
  TEST_SPECIFIC_STYLE_WRITER(Overflow);
  TEST_SPECIFIC_STYLE_WRITER(OverflowX);
  TEST_SPECIFIC_STYLE_WRITER(OverflowY);

  style_.text_attributes_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_ = starlight::TextAttributes(2);
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_size = 10;
  TEST_SPECIFIC_STYLE_WRITER(FontSize);

  style_.text_attributes_->font_size = -10;
  TEST_SPECIFIC_STYLE_WRITER(FontSize);

  style_.text_attributes_->computed_line_height = 100;
  TEST_SPECIFIC_STYLE_WRITER(LineHeight);

  style_.text_attributes_->computed_line_height = -100;
  TEST_SPECIFIC_STYLE_WRITER(LineHeight);

  style_.text_attributes_->letter_spacing = 100;
  TEST_SPECIFIC_STYLE_WRITER(LetterSpacing);

  style_.text_attributes_->letter_spacing = -100;
  TEST_SPECIFIC_STYLE_WRITER(LetterSpacing);

  style_.text_attributes_->line_spacing = 100;
  TEST_SPECIFIC_STYLE_WRITER(LineSpacing);

  style_.text_attributes_->line_spacing = -100;
  TEST_SPECIFIC_STYLE_WRITER(LineSpacing);

  style_.text_attributes_->text_gradient = lepus::Value();
  TEST_SPECIFIC_STYLE_WRITER(Color);

  style_.text_attributes_->text_gradient =
      lepus::Value(lepus::CArray::Create());
  TEST_SPECIFIC_STYLE_WRITER(Color);

  style_.text_attributes_->text_gradient->SetProperty(0, lepus::Value(2));
  TEST_SPECIFIC_STYLE_WRITER(Color);

  style_.text_attributes_->text_gradient->SetProperty(1, lepus::Value(3));
  TEST_SPECIFIC_STYLE_WRITER(Color);

  style_.text_attributes_->text_gradient = lepus::Value();
  TEST_SPECIFIC_STYLE_WRITER(Color);

  style_.background_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.background_data_ = starlight::BackgroundData();
  style_.background_data_->image_data.emplace();
  style_.background_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kPaddingBox);
  style_.background_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kBorderBox);
  style_.background_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kContentBox);
  style_.background_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kText);
  TEST_SPECIFIC_STYLE_WRITER(BackgroundClip);

  style_.background_data_->color = 200;
  TEST_SPECIFIC_STYLE_WRITER(BackgroundColor);

  style_.background_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kPaddingBox);
  style_.background_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kBorderBox);
  style_.background_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kContentBox);
  style_.background_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kBorderBox);
  TEST_SPECIFIC_STYLE_WRITER(BackgroundOrigin);

  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeat);
  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kNoRepeat);
  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeatX);
  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeatY);
  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRound);
  style_.background_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kSpace);
  TEST_SPECIFIC_STYLE_WRITER(BackgroundRepeat);

  style_.background_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.mask_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.mask_data_ = starlight::BackgroundData();
  style_.mask_data_->image_data.emplace();
  style_.mask_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kPaddingBox);
  style_.mask_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kBorderBox);
  style_.mask_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kContentBox);
  style_.mask_data_->image_data->clip.emplace_back(
      starlight::BackgroundClipType::kText);
  TEST_SPECIFIC_STYLE_WRITER(MaskClip);

  style_.mask_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kPaddingBox);
  style_.mask_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kBorderBox);
  style_.mask_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kContentBox);
  style_.mask_data_->image_data->origin.emplace_back(
      starlight::BackgroundOriginType::kBorderBox);
  TEST_SPECIFIC_STYLE_WRITER(MaskOrigin);

  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeat);
  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kNoRepeat);
  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeatX);
  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRepeatY);
  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kRound);
  style_.mask_data_->image_data->repeat.emplace_back(
      starlight::BackgroundRepeatType::kSpace);
  TEST_SPECIFIC_STYLE_WRITER(MaskRepeat);

  style_.mask_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.surround_data_.border_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.surround_data_.border_data_ =
      starlight::BordersData();
  style_.layout_computed_style_.surround_data_.border_data_->color_left = 6;
  style_.layout_computed_style_.surround_data_.border_data_->color_right = 7;
  style_.layout_computed_style_.surround_data_.border_data_->color_top = 8;
  style_.layout_computed_style_.surround_data_.border_data_->color_bottom = 9;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.surround_data_.border_data_->width_left = 16;
  style_.layout_computed_style_.surround_data_.border_data_->width_top = 17;
  style_.layout_computed_style_.surround_data_.border_data_->width_right = 18;
  style_.layout_computed_style_.surround_data_.border_data_->width_bottom = 19;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.surround_data_.border_data_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.animation_data_->front().duration = 26;
  style_.animation_data_->front().delay = 27;
  style_.animation_data_->front().iteration_count = 28;
  style_.animation_data_->front().direction =
      starlight::AnimationDirectionType::kAlternate;
  style_.animation_data_->front().fill_mode =
      starlight::AnimationFillModeType::kForwards;
  style_.animation_data_->front().play_state =
      starlight::AnimationPlayStateType::kPaused;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_animation_data_->create_ani.duration = 36;
  style_.layout_animation_data_->create_ani.delay = 37;
  style_.layout_animation_data_->create_ani.duration = 38;
  style_.layout_animation_data_->create_ani.property =
      starlight::AnimationPropertyType::kLeft;

  style_.layout_animation_data_->delete_ani.duration = 46;
  style_.layout_animation_data_->delete_ani.delay = 47;
  style_.layout_animation_data_->delete_ani.duration = 48;
  style_.layout_animation_data_->delete_ani.property =
      starlight::AnimationPropertyType::kRight;

  style_.layout_animation_data_->update_ani.duration = 56;
  style_.layout_animation_data_->update_ani.delay = 57;
  style_.layout_animation_data_->update_ani.duration = 58;
  style_.layout_animation_data_->update_ani.property =
      starlight::AnimationPropertyType::kTop;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.visibility_ = starlight::VisibilityType::kCollapse;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.visibility_ = starlight::VisibilityType::kVisible;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.surround_data_.border_data_ =
      starlight::BordersData();
  style_.layout_computed_style_.surround_data_.border_data_->style_left =
      starlight::BorderStyleType::kGroove;
  style_.layout_computed_style_.surround_data_.border_data_->style_right =
      starlight::BorderStyleType::kDotted;
  style_.layout_computed_style_.surround_data_.border_data_->style_top =
      starlight::BorderStyleType::kRidge;
  style_.layout_computed_style_.surround_data_.border_data_->style_bottom =
      starlight::BorderStyleType::kUndefined;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.outline_->color = 66;
  style_.outline_->style = starlight::BorderStyleType::kDouble;
  style_.outline_->width = 67;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_family = base::String("test");
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_.reset();
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.caret_color_ = base::String("test0");
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.direction_ = starlight::DirectionType::kLynxRtl;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.direction_ = starlight::DirectionType::kNormal;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.direction_ = starlight::DirectionType::kRtl;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.layout_computed_style_.direction_ = starlight::DirectionType::kLtr;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_ = starlight::TextAttributes(2);
  style_.text_attributes_->white_space = starlight::WhiteSpaceType::kNowrap;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_weight = starlight::FontWeightType::k800;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_weight = starlight::FontWeightType::k300;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->word_break = starlight::WordBreakType::kKeepAll;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->word_break = starlight::WordBreakType::kBreakAll;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_style = starlight::FontStyleType::kItalic;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->font_style = starlight::FontStyleType::kOblique;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->text_align = starlight::TextAlignType::kRight;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->text_align = starlight::TextAlignType::kJustify;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->text_overflow =
      starlight::TextOverflowType::kEllipsis;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.text_attributes_->decoration_color = 88;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.z_index_ = 98;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);

  style_.image_rendering_ = starlight::ImageRenderingType::kCrispEdges;
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);
  style_.layout_computed_style_.linear_data_.Access()->list_main_axis_gap_ =
      starlight::NLength::MakeMaxContentNLength();
  style_.layout_computed_style_.linear_data_.Access()->list_cross_axis_gap_ =
      starlight::NLength::MakeUnitNLength(108);
  TEST_SPECIFIC_STYLE_WRITER(ListMainAxisGap);
  TEST_SPECIFIC_STYLE_WRITER(ListCrossAxisGap);

  style_.cursor_ = lepus::Value(lepus::CArray::Create());
  style_.cursor_->SetProperty(0, lepus::Value(1));
  style_.cursor_->SetProperty(0, lepus::Value(2));
  TEST_SPECIFIC_STYLE_WRITER(TextStroke);

  style_.text_attributes_->text_stroke_width = 128;
  TEST_SPECIFIC_STYLE_WRITER(TextStrokeWidth);

  style_.text_attributes_->text_stroke_color = 138;
  TEST_SPECIFIC_STYLE_WRITER(TextStrokeColor);

  style_.text_attributes_->hyphens = starlight::HyphensType::kManual;
  TEST_SPECIFIC_STYLE_WRITER(Hyphens);

  style_.app_region_ = starlight::XAppRegionType::kDrag;
  TEST_SPECIFIC_STYLE_WRITER(XAppRegion);

  style_.handle_size_ = 148;
  TEST_SPECIFIC_STYLE_WRITER(XHandleSize);

  style_.handle_color_ = 158;
  TEST_SPECIFIC_STYLE_WRITER(XHandleColor);
  FOREACH_PLATFORM_PROPERTY(TEST_SPECIFIC_STYLE_WRITER);
}

#undef TEST_SPECIFIC_STYLE_WRITER

}  // namespace test
}  // namespace tasm
}  // namespace lynx
