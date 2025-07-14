// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/include/starlight_standalone/starlight.h"

#include "base/include/no_destructor.h"
#include "core/include/starlight_standalone/starlight_enums.h"
#include "core/include/starlight_standalone/starlight_value.h"
#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/types/layout_configs.h"

#define GET_INNER_LAYOUT_NODE(a) \
  reinterpret_cast<lynx::starlight::LayoutObject *>(a)
#define GET_OUTER_LAYOUT_NODE(a) reinterpret_cast<StarlightNode *>(a)

static lynx::starlight::Direction ResolveEdgeToDirection(
    lynx::starlight::LayoutObject *const node, SLEdge edge) {
  const bool is_rtl = node->GetCSSStyle()->IsRtl();
  switch (edge) {
    case SLEdgeLeft:
      return lynx::starlight::Direction::kLeft;
    case SLEdgeRight:
      return lynx::starlight::Direction::kRight;
    case SLEdgeTop:
      return lynx::starlight::Direction::kTop;
    case SLEdgeBottom:
      return lynx::starlight::Direction::kBottom;
    case SLEdgeStart:
      return is_rtl ? lynx::starlight::Direction::kRight
                    : lynx::starlight::Direction::kLeft;
    case SLEdgeEnd:
      return is_rtl ? lynx::starlight::Direction::kLeft
                    : lynx::starlight::Direction::kRight;
    default:
      return lynx::starlight::Direction::kLeft;
  }
}

static struct StarlightValue NLengthToStarlightValue(
    const lynx::starlight::NLength &length) {
  struct StarlightValue result;
  switch (length.GetType()) {
    case lynx::starlight::NLengthType::kNLengthAuto:
      result.unit_ = SLUnitAuto;
      break;
    case lynx::starlight::NLengthType::kNLengthUnit:
      result.value_ = length.GetRawValue();
      result.unit_ = SLUnitPoint;
      break;
    case lynx::starlight::NLengthType::kNLengthPercentage:
      result.value_ = length.GetRawValue();
      result.unit_ = SLUnitPercent;
      break;
    case lynx::starlight::NLengthType::kNLengthMaxContent:
      result.unit_ = SLUnitMaxContent;
      break;
    case lynx::starlight::NLengthType::kNLengthFitContent:
      result.unit_ = SLUnitFitContent;
      break;
    default:
      break;
  }
  return result;
}

static lynx::starlight::LayoutConfigs CreateDefaultLayoutConfigs() {
  lynx::starlight::LayoutConfigs config;
  config.SetQuirksMode(lynx::kAbsoluteAndFixedBoxInfoFixedVersion);
  config.SetTargetSDKVersion(kStarlightDefaultTargetSDKVersion);
  return config;
}

static const lynx::starlight::LayoutConfigs &GetDefaultLayoutConfigs() {
  static const lynx::base::NoDestructor<lynx::starlight::LayoutConfigs>
      kDefaultLayoutConfigs(CreateDefaultLayoutConfigs());
  return *kDefaultLayoutConfigs;
}

static const lynx::starlight::LayoutComputedStyle &GetDefaultStyle() {
  static const lynx::base::NoDestructor<lynx::starlight::LayoutComputedStyle>
      kDefaultStyle(kDefaultPhysicalPixelsPerLayoutUnit);
  return *kDefaultStyle;
}

SLNodeRef SLNodeNew() {
  lynx::starlight::LayoutObject *node = new lynx::starlight::LayoutObject(
      GetDefaultLayoutConfigs(),
      new lynx::starlight::LayoutComputedStyle(GetDefaultStyle()));
  return GET_OUTER_LAYOUT_NODE(node);
}

SLNodeRef SLNodeNewWithConfig(StarlightConfig *config) {
  lynx::starlight::LayoutComputedStyle *const css_style =
      new lynx::starlight::LayoutComputedStyle(GetDefaultStyle());
  css_style->SetPhysicalPixelsPerLayoutUnit(
      SLConfigGetPhysicalPixelsPerLayoutUnit(config));
  lynx::starlight::LayoutObject *node =
      new lynx::starlight::LayoutObject(GetDefaultLayoutConfigs(), css_style);
  return GET_OUTER_LAYOUT_NODE(node);
}

void SLNodeInsertChild(const SLNodeRef parent, const SLNodeRef child,
                       int32_t index) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  lynx::starlight::LayoutObject *child_node = GET_INNER_LAYOUT_NODE(child);
  if (lynx::starlight::LayoutObject *original_parent =
          child_node->ParentLayoutObject()) {
    original_parent->RemoveChild(child_node);
  }
  if (index == -1) {
    parent_node->AppendChild(child_node);
  } else {
    parent_node->InsertChildBefore(
        child_node,
        static_cast<lynx::starlight::LayoutObject *>(parent_node->Find(index)));
  }
  parent_node->MarkDirty();
}

void SLNodeRemoveChild(const SLNodeRef parent, const SLNodeRef child) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  lynx::starlight::LayoutObject *child_node = GET_INNER_LAYOUT_NODE(child);
  if (parent_node == child_node->ParentLayoutObject()) {
    parent_node->RemoveChild(child_node);
    parent_node->MarkDirty();
  }
}

void SLNodeRemoveAllChildren(const SLNodeRef parent) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  while (parent_node->GetChildCount() > 0) {
    lynx::starlight::LayoutObject *child =
        static_cast<lynx::starlight::LayoutObject *>(parent_node->FirstChild());
    SLNodeRemoveChild(GET_OUTER_LAYOUT_NODE(parent_node),
                      GET_OUTER_LAYOUT_NODE(child));
  }
}

SLNodeRef SLNodeGetChild(const SLNodeRef node, int32_t index) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  if (index < inner_node->GetChildCount()) {
    return GET_OUTER_LAYOUT_NODE(inner_node->Find(index));
  }
  return nullptr;
}

int32_t SLNodeGetChildCount(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetChildCount();
}

SLNodeRef SLNodeGetParent(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  return GET_OUTER_LAYOUT_NODE(inner_node->ParentLayoutObject());
}

void SLNodeFree(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node == nullptr) {
    return;
  }
  if (auto *css_style = inner_node->GetCSSMutableStyle()) {
    delete css_style;
  }
  delete inner_node;
}

void SLNodeFreeRecursive(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  while (inner_node->GetChildCount() > 0) {
    lynx::starlight::LayoutObject *const child =
        static_cast<lynx::starlight::LayoutObject *>(inner_node->FirstChild());
    SLNodeFreeRecursive(GET_OUTER_LAYOUT_NODE(child));
  }
  SLNodeFree(GET_OUTER_LAYOUT_NODE(inner_node));
}

void SLNodeReset(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->Reset(inner_node);
}

bool SLNodeIsDirty(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->IsDirty();
}

void SLNodeMarkDirty(const SLNodeRef node) {
  GET_INNER_LAYOUT_NODE(node)->MarkDirty();
}

static void SLNodeMarkNotDirtyRecursive(lynx::starlight::LayoutObject *node) {
  node->MarkNotDirty();
  lynx::starlight::LayoutObject *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    SLNodeMarkNotDirtyRecursive(child);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

bool SLNodeIsRTL(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetCSSStyle()->IsRtl();
}

static void SLNodeHandleRTLRecursive(lynx::starlight::LayoutObject *const node,
                                     bool is_rtl) {
  lynx::starlight::LayoutComputedStyle *const style =
      node->GetCSSMutableStyle();
  if (style->GetDirection() == lynx::starlight::DirectionType::kNormal) {
    auto direction = is_rtl ? lynx::starlight::DirectionType::kRtl
                            : lynx::starlight::DirectionType::kLtr;
    style->SetDirection(direction);
  }
  lynx::starlight::LayoutObject *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    SLNodeHandleRTLRecursive(child, is_rtl);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

void SLNodeCalculateLayout(const SLNodeRef node, float owner_width,
                           float owner_height, SLDirection owner_direction) {
  // containing block
  lynx::starlight::Constraints owner_constraints;
  owner_constraints[SLHorizontal] =
      owner_width == SLUndefined
          ? lynx::starlight::OneSideConstraint::Indefinite()
          : lynx::starlight::OneSideConstraint::Definite(owner_width);
  owner_constraints[SLVertical] =
      owner_height == SLUndefined
          ? lynx::starlight::OneSideConstraint::Indefinite()
          : lynx::starlight::OneSideConstraint::Definite(owner_height);

  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->MarkDirty();
  inner_node->GetBoxInfo()->InitializeBoxInfo(owner_constraints, *inner_node,
                                              inner_node->GetLayoutConfigs());
  lynx::starlight::Constraints constraints =
      lynx::starlight::property_utils::GenerateDefaultConstraints(
          *inner_node, owner_constraints);

  // handle RTL, if the direction of node is not setting, set the
  // owner_direction to the node.
  if (owner_direction == SLDirection::SLDirectionRTL) {
    SLNodeHandleRTLRecursive(inner_node,
                             owner_direction == SLDirection::SLDirectionRTL);
  }

  inner_node->ReLayoutWithConstraints(constraints);
  SLNodeMarkNotDirtyRecursive(inner_node);
}

StarlightMeasureDelegate *SLNodeGetContext(const SLNodeRef node) {
  return static_cast<StarlightMeasureDelegate *>(
      GET_INNER_LAYOUT_NODE(node)->GetContext());
}

void SLNodeSetContext(const SLNodeRef node,
                      StarlightMeasureDelegate *const delegate) {
  GET_INNER_LAYOUT_NODE(node)->SetContext(static_cast<void *>(delegate));
}

void SLNodeSetMeasureFunc(const SLNodeRef node,
                          StarlightMeasureDelegate *const delegate) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->SetContext(delegate);
  if (delegate) {
    inner_node->SetSLMeasureFunc(
        [](void *context, const lynx::starlight::Constraints &constraints,
           bool final_measure) {
          StarlightMeasureDelegate *const measure_delegate =
              static_cast<StarlightMeasureDelegate *>(context);
          const SLNodeMeasureMode width_mode =
              static_cast<SLNodeMeasureMode>(constraints[SLHorizontal].Mode());
          const float width =
              width_mode == SLNodeMeasureMode::SLNodeMeasureModeUndefined
                  ? 0.f
                  : constraints[SLHorizontal].Size();
          const SLNodeMeasureMode height_mode =
              static_cast<SLNodeMeasureMode>(constraints[SLVertical].Mode());
          const float height =
              height_mode == SLNodeMeasureMode::SLNodeMeasureModeUndefined
                  ? 0.f
                  : constraints[SLVertical].Size();
          StarlightSize size;
          float baseline = 0.f;
          if (measure_delegate->measure_func_) {
            // If width and height are both exact, use the width and height
            // from the constraints, and don't call the measure_func_.
            if (width_mode == SLNodeMeasureMode::SLNodeMeasureModeExactly &&
                height_mode == SLNodeMeasureMode::SLNodeMeasureModeExactly) {
              size.width_ = width;
              size.height_ = height;
            } else {
              size = measure_delegate->measure_func_(
                  measure_delegate->instance_, width, width_mode, height,
                  height_mode);
            }
          }
          if (measure_delegate->baseline_func_) {
            baseline = measure_delegate->baseline_func_(
                measure_delegate->instance_, width, width_mode, height,
                height_mode);
          }
          return FloatSize{size.width_, size.height_, baseline};
        });
  } else {
    inner_node->SetSLMeasureFunc(nullptr);
  }
}

bool SLNodeHasMeasureFunc(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetSLMeasureFunc() != nullptr;
}

// Styles
void SLNodeStyleSetDirection(const SLNodeRef node, SLDirection type) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetDirection(
          static_cast<lynx::starlight::DirectionType>(type))) {
    inner_node->MarkDirty();
  }
}

#define SET_ENUM_STYLE(type_name, enum_type, inner_enum_type, set_method) \
  void SLNodeStyleSet##type_name(const SLNodeRef node, enum_type value) { \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    if (inner_node->GetCSSMutableStyle()->set_method(                     \
            static_cast<lynx::starlight::inner_enum_type>(value))) {      \
      inner_node->MarkDirty();                                            \
    }                                                                     \
  }

#define SUPPORTED_ENUM_STYLE_SETTER(V)                                   \
  V(FlexDirection, SLFlexDirection, FlexDirectionType, SetFlexDirection) \
  V(AlignContent, SLAlignContent, AlignContentType, SetAlignContent)     \
  V(AlignSelf, SLFlexAlign, FlexAlignType, SetAlignSelf)                 \
  V(PositionType, SLPositionType, PositionType, SetPosition)             \
  V(FlexWrap, SLFlexWrap, FlexWrapType, SetFlexWrap)                     \
  V(Display, SLDisplay, DisplayType, SetDisplay)                         \
  V(BoxSizing, SLBoxSizing, BoxSizingType, SetBoxSizing)

SUPPORTED_ENUM_STYLE_SETTER(SET_ENUM_STYLE)

#undef SUPPORTED_ENUM_STYLE_SETTER
#undef SET_ENUM_STYLE

// alignment
void SLNodeStyleSetJustifyContent(const SLNodeRef node,
                                  SLJustifyContent value) {
  lynx::starlight::JustifyContentType type;
  switch (value) {
    case SLJustifyContent::SLJustifyContentFlexStart:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
    case SLJustifyContent::SLJustifyContentCenter:
      type = lynx::starlight::JustifyContentType::kCenter;
      break;
    case SLJustifyContent::SLJustifyContentFlexEnd:
      type = lynx::starlight::JustifyContentType::kFlexEnd;
      break;
    case SLJustifyContent::SLJustifyContentSpaceBetween:
      type = lynx::starlight::JustifyContentType::kSpaceBetween;
      break;
    case SLJustifyContent::SLJustifyContentSpaceAround:
      type = lynx::starlight::JustifyContentType::kSpaceAround;
      break;
    case SLJustifyContent::SLJustifyContentSpaceEvenly:
      type = lynx::starlight::JustifyContentType::kSpaceEvenly;
      break;
    case SLJustifyContent::SLJustifyContentStart:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
    case SLJustifyContent::SLJustifyContentEnd:
      type = lynx::starlight::JustifyContentType::kFlexEnd;
      break;
    default:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
  }
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetJustifyContent(type)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetAlignItems(const SLNodeRef node, SLFlexAlign value) {
  // auto is not supported in align-items
  if (value == SLFlexAlign::SLFlexAlignAuto) {
    return;
  }
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetAlignItems(
          static_cast<lynx::starlight::FlexAlignType>(value))) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetAspectRatio(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetAspectRatio(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetOrder(const SLNodeRef node, int32_t value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetOrder(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetFlexGrow(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetFlexGrow(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetFlexShrink(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetFlexShrink(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetFlex(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  bool need_mark_dirty = false;
  need_mark_dirty = inner_node->GetCSSMutableStyle()->SetFlexGrow(value);
  need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetFlexShrink(1);
  need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetFlexBasis(
      lynx::starlight::NLength::MakeUnitNLength(0));
  if (need_mark_dirty) {
    inner_node->MarkDirty();
  }
}

#define DEFINE_EDGE_STYLE_SETTER(func_suffix, css_method_prefix,          \
                                 nlength_maker)                           \
  void SLNodeStyleSet##func_suffix(const SLNodeRef node, SLEdge edge,     \
                                   float value) {                         \
    bool need_mark_dirty = false;                                         \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    auto *css_style = inner_node->GetCSSMutableStyle();                   \
    const bool is_rtl = inner_node->GetCSSStyle()->IsRtl();               \
                                                                          \
    switch (edge) {                                                       \
      case SLEdgeLeft:                                                    \
        need_mark_dirty = css_style->Set##css_method_prefix##Left(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeRight:                                                   \
        need_mark_dirty = css_style->Set##css_method_prefix##Right(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeTop:                                                     \
        need_mark_dirty = css_style->Set##css_method_prefix##Top(         \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeBottom:                                                  \
        need_mark_dirty = css_style->Set##css_method_prefix##Bottom(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeStart:                                                   \
        need_mark_dirty =                                                 \
            is_rtl ? css_style->Set##css_method_prefix##Right(            \
                         lynx::starlight::NLength::nlength_maker(value))  \
                   : css_style->Set##css_method_prefix##Left(             \
                         lynx::starlight::NLength::nlength_maker(value)); \
        break;                                                            \
      case SLEdgeEnd:                                                     \
        need_mark_dirty =                                                 \
            is_rtl ? css_style->Set##css_method_prefix##Left(             \
                         lynx::starlight::NLength::nlength_maker(value))  \
                   : css_style->Set##css_method_prefix##Right(            \
                         lynx::starlight::NLength::nlength_maker(value)); \
        break;                                                            \
      case SLEdgeHorizontal:                                              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeVertical:                                                \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(     \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeAll:                                                     \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(     \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      default:                                                            \
        break;                                                            \
    }                                                                     \
    if (need_mark_dirty) {                                                \
      inner_node->MarkDirty();                                            \
    }                                                                     \
  }

// top, bottom, left, right
// SLNodeStyleSetPosition
DEFINE_EDGE_STYLE_SETTER(Position, , MakeUnitNLength)
// SLNodeStyleSetPositionPercent
DEFINE_EDGE_STYLE_SETTER(PositionPercent, , MakePercentageNLength)
// SLNodeStyleSetMargin
DEFINE_EDGE_STYLE_SETTER(Margin, Margin, MakeUnitNLength)
// SLNodeStyleSetMarginPercent
DEFINE_EDGE_STYLE_SETTER(MarginPercent, Margin, MakePercentageNLength)
// SLNodeStyleSetPadding
DEFINE_EDGE_STYLE_SETTER(Padding, Padding, MakeUnitNLength)
// SLNodeStyleSetPaddingPercent
DEFINE_EDGE_STYLE_SETTER(PaddingPercent, Padding, MakePercentageNLength)

#undef DEFINE_EDGE_STYLE_SETTER

void SLNodeStyleSetMarginAuto(const SLNodeRef node, SLEdge edge) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetMarginLeft(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetMarginRight(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetMarginTop(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetMarginBottom(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetMarginRight(
            lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty = css_style->SetMarginLeft(
            lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetMarginLeft(
            lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty = css_style->SetMarginRight(
            lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty =
          css_style->SetMarginLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginRight(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty =
          css_style->SetMarginTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginBottom(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty =
          css_style->SetMarginLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginRight(
          lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetMarginTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginBottom(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

void SLNodeStyleSetPositionAuto(const SLNodeRef node, SLEdge edge) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty =
            css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty =
            css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty =
            css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty =
            css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty =
          css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty =
          css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty =
          css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

// border width
void SLNodeStyleSetBorder(const SLNodeRef node, SLEdge edge, float value) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetBorderLeftWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetBorderRightWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetBorderTopWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetBorderBottomWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetBorderRightWidth(value);
      } else {
        need_mark_dirty = css_style->SetBorderLeftWidth(value);
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetBorderLeftWidth(value);
      } else {
        need_mark_dirty = css_style->SetBorderRightWidth(value);
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty = css_style->SetBorderLeftWidth(value);
      need_mark_dirty |= css_style->SetBorderRightWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty = css_style->SetBorderTopWidth(value);
      need_mark_dirty |= css_style->SetBorderBottomWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty = css_style->SetBorderTopWidth(value);
      need_mark_dirty |= css_style->SetBorderBottomWidth(value);
      need_mark_dirty |= css_style->SetBorderLeftWidth(value);
      need_mark_dirty |= css_style->SetBorderRightWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

void SLNodeStyleSetGap(const SLNodeRef node, SLGap gap, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  switch (gap) {
    case SLGapColumn:
      if (inner_node->GetCSSMutableStyle()->SetColumnGap(
              lynx::starlight::NLength::MakeUnitNLength(value))) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapRow:
      if (inner_node->GetCSSMutableStyle()->SetRowGap(
              lynx::starlight::NLength::MakeUnitNLength(value))) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapAll: {
      bool need_mark_dirty = false;
      need_mark_dirty = inner_node->GetCSSMutableStyle()->SetColumnGap(
          lynx::starlight::NLength::MakeUnitNLength(value));
      need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetRowGap(
          lynx::starlight::NLength::MakeUnitNLength(value));
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    }
  }
}

void SLNodeStyleSetGapPercent(const SLNodeRef node, SLGap gap, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  switch (gap) {
    case SLGapColumn:
      if (inner_node->GetCSSMutableStyle()->SetColumnGap(
              lynx::starlight::NLength::MakePercentageNLength(value))) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapRow:
      if (inner_node->GetCSSMutableStyle()->SetRowGap(
              lynx::starlight::NLength::MakePercentageNLength(value))) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapAll: {
      bool need_mark_dirty = false;
      need_mark_dirty = inner_node->GetCSSMutableStyle()->SetColumnGap(
          lynx::starlight::NLength::MakePercentageNLength(value));
      need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetRowGap(
          lynx::starlight::NLength::MakePercentageNLength(value));
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
    } break;
  }
}

// set size styles with value, e.g., width: 100%, height: max-content,
// min-width: 10%.
#define SET_SIZE_STYLE_WITH_VALUE(type_name, type_pre_fix, length_type)  \
  void SLNodeStyleSet##type_pre_fix(const SLNodeRef node, float value) { \
    lynx::starlight::LayoutObject *const inner_node =                    \
        GET_INNER_LAYOUT_NODE(node);                                     \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(                \
            lynx::starlight::NLength::Make##length_type(value))) {       \
      inner_node->MarkDirty();                                           \
    }                                                                    \
  }

#define SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER(V)   \
  V(Width, Width, UnitNLength)                      \
  V(Width, WidthPercent, PercentageNLength)         \
  V(MinWidth, MinWidth, UnitNLength)                \
  V(MinWidth, MinWidthPercent, PercentageNLength)   \
  V(MaxWidth, MaxWidth, UnitNLength)                \
  V(MaxWidth, MaxWidthPercent, PercentageNLength)   \
  V(Height, Height, UnitNLength)                    \
  V(Height, HeightPercent, PercentageNLength)       \
  V(MinHeight, MinHeight, UnitNLength)              \
  V(MinHeight, MinHeightPercent, PercentageNLength) \
  V(MaxHeight, MaxHeight, UnitNLength)              \
  V(MaxHeight, MaxHeightPercent, PercentageNLength) \
  V(FlexBasis, FlexBasis, UnitNLength)              \
  V(FlexBasis, FlexBasisPercent, PercentageNLength)

SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER(SET_SIZE_STYLE_WITH_VALUE)

#undef SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER
#undef SET_SIZE_STYLE_WITH_VALUE

// set size styles with no value param, e.g., width: auto, height: max-content,
// flex-basis: auto.
#define SET_SIZE_STYLE_WITH_NO_VALUE_PARAM(type_name, type_pre_fix, \
                                           length_type)             \
  void SLNodeStyleSet##type_pre_fix(const SLNodeRef node) {         \
    lynx::starlight::LayoutObject *const inner_node =               \
        GET_INNER_LAYOUT_NODE(node);                                \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(           \
            lynx::starlight::NLength::Make##length_type())) {       \
      inner_node->MarkDirty();                                      \
    }                                                               \
  }

#define SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER(V) \
  V(Width, WidthAuto, AutoNLength)                                    \
  V(Width, WidthMaxContent, MaxContentNLength)                        \
  V(Width, WidthFitContent, FitContentNLength)                        \
  V(Height, HeightAuto, AutoNLength)                                  \
  V(Height, HeightMaxContent, MaxContentNLength)                      \
  V(Height, HeightFitContent, FitContentNLength)                      \
  V(FlexBasis, FlexBasisAuto, AutoNLength)

SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER(
    SET_SIZE_STYLE_WITH_NO_VALUE_PARAM)

#undef SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER
#undef SET_SIZE_STYLE_WITH_NO_VALUE_PARAM

// get styles with eunm type getter, e.g., flex-direction, justify-content.
#define GET_ENUM_STYLE(type_name, return_type, get_expr)                  \
  return_type SLNodeStyleGet##type_name(const SLNodeRef node) {           \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    return static_cast<return_type>(inner_node->GetCSSStyle()->get_expr); \
  }

#define SUPPORTED_ENUM_STYLE_GETTER(V)                     \
  V(FlexDirection, SLFlexDirection, GetFlexDirection())    \
  V(JustifyContent, SLJustifyContent, GetJustifyContent()) \
  V(AlignContent, SLAlignContent, GetAlignContent())       \
  V(AlignItems, SLFlexAlign, GetAlignItems())              \
  V(AlignSelf, SLFlexAlign, GetAlignSelf())                \
  V(PositionType, SLPositionType, GetPosition())           \
  V(FlexWrap, SLFlexWrap, GetFlexWrap())                   \
  V(Display, SLDisplay, display_)                          \
  V(BoxSizing, SLBoxSizing, box_sizing_)

SUPPORTED_ENUM_STYLE_GETTER(GET_ENUM_STYLE)

#undef SUPPORTED_ENUM_STYLE_GETTER
#undef GET_ENUM_STYLE

// get styles with basic type getter, e.g., aspect-ratio, order.
#define GET_BASIC_TYPE_STYLE(type_name, return_type, get_expr)  \
  return_type SLNodeStyleGet##type_name(const SLNodeRef node) { \
    lynx::starlight::LayoutObject *const inner_node =           \
        GET_INNER_LAYOUT_NODE(node);                            \
    return inner_node->GetCSSStyle()->get_expr;                 \
  }

#define SUPPORTED_BASIC_TYPE_STYLE_GETTER(V)      \
  V(AspectRatio, float, box_data_->aspect_ratio_) \
  V(Order, int32_t, GetOrder())                   \
  V(FlexGrow, float, GetFlexGrow())               \
  V(FlexShrink, float, GetFlexShrink())

SUPPORTED_BASIC_TYPE_STYLE_GETTER(GET_BASIC_TYPE_STYLE)

#undef SUPPORTED_ENUM_STYLE_GETTER
#undef GET_BASIC_TYPE_STYLE

// get styles with length type getter, e.g., width, height.
#define GET_LENGTH_STYLE(type_name, get_expr)                             \
  struct StarlightValue SLNodeStyleGet##type_name(const SLNodeRef node) { \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    return NLengthToStarlightValue(inner_node->GetCSSStyle()->get_expr);  \
  }

#define SUPPORTED_LENGTH_STYLE_GETTER(V) \
  V(FlexBasis, GetFlexBasis())           \
  V(Width, GetWidth())                   \
  V(Height, GetHeight())                 \
  V(MinWidth, GetMinWidth())             \
  V(MaxWidth, GetMaxWidth())             \
  V(MinHeight, GetMinHeight())           \
  V(MaxHeight, GetMaxHeight())

SUPPORTED_LENGTH_STYLE_GETTER(GET_LENGTH_STYLE)

#undef SUPPORTED_LENGTH_STYLE_GETTER
#undef GET_LENGTH_STYLE

// get padding, margin, position styles with edge type getter.
#define DEFINE_EDGE_STYLE_GETTER(StyleType, GetterPrefix)                   \
  struct StarlightValue SLNodeStyleGet##StyleType(const SLNodeRef node,     \
                                                  SLEdge edge) {            \
    lynx::starlight::LayoutObject *const inner_node =                       \
        GET_INNER_LAYOUT_NODE(node);                                        \
    switch (edge) {                                                         \
      case SLEdgeLeft:                                                      \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Left());               \
      case SLEdgeRight:                                                     \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Right());              \
      case SLEdgeTop:                                                       \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Top());                \
      case SLEdgeBottom:                                                    \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Bottom());             \
      case SLEdgeStart:                                                     \
        return SLNodeIsRTL(node)                                            \
                   ? NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Right())  \
                   : NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Left());  \
      case SLEdgeEnd:                                                       \
        return SLNodeIsRTL(node)                                            \
                   ? NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Left())   \
                   : NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Right()); \
      default:                                                              \
        return (struct StarlightValue){0.0f, SLUnitPoint};                  \
    }                                                                       \
  }

DEFINE_EDGE_STYLE_GETTER(Position, Get)
DEFINE_EDGE_STYLE_GETTER(Margin, GetMargin)
DEFINE_EDGE_STYLE_GETTER(Padding, GetPadding)

#undef DEFINE_EDGE_STYLE_GETTER

struct StarlightValue SLNodeStyleGetGap(const SLNodeRef node, SLGap gap) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  switch (gap) {
    case SLGapColumn:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridColumnGap());
    case SLGapRow:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridRowGap());
    case SLGapAll:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridRowGap());
    default:
      return (struct StarlightValue){0.0f, SLUnitPoint};
  }
}

float SLNodeStyleGetBorder(const SLNodeRef node, SLEdge edge) {
  return SLNodeLayoutGetBorder(node, edge);
}

float SLNodeLayoutGetLeft(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.offset_.X();
}

float SLNodeLayoutGetTop(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.offset_.Y();
}

// TODO
// float SLNodeLayoutGetRight(lynx::starlight::LayoutObject * const node) {
//   const auto &result = node->GetLayoutResult();
//   return result.offset_.X();
// }

// TODO
// float SLNodeLayoutGetBottom(lynx::starlight::LayoutObject * const node) {
//   const auto &result = node->GetLayoutResult();
//   return result.offset_.Y();
// }

float SLNodeLayoutGetWidth(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.size_.width_;
}

float SLNodeLayoutGetHeight(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.size_.height_;
}

float SLNodeLayoutGetMargin(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.margin_[ResolveEdgeToDirection(inner_node, edge)];
}

float SLNodeLayoutGetPadding(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.padding_[ResolveEdgeToDirection(inner_node, edge)];
}

float SLNodeLayoutGetBorder(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.border_[ResolveEdgeToDirection(inner_node, edge)];
}
