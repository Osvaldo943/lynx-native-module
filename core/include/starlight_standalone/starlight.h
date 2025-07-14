// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_
#define CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_

#include <stdint.h>

#include "starlight_config.h"
#include "starlight_value.h"

#ifdef __cplusplus
extern "C" {
#endif

// a wrapper of LayoutObject.
struct StarlightNode;
typedef struct StarlightNode* SLNodeRef;
struct StarlightSize;
struct StarlightConfig;

typedef struct StarlightSize (*StarlightMeasureFunc)(
    void* instance, float width, SLNodeMeasureMode width_mode, float height,
    SLNodeMeasureMode height_mode);

typedef float (*StarlightBaselineFunc)(void* instance, float width,
                                       SLNodeMeasureMode width_mode,
                                       float height,
                                       SLNodeMeasureMode height_mode);

typedef struct StarlightMeasureDelegate {
  StarlightMeasureFunc measure_func_;
  StarlightBaselineFunc baseline_func_;
  void* instance_;
} StarlightMeasureDelegate;

SLNodeRef SLNodeNew();
SLNodeRef SLNodeNewWithConfig(StarlightConfig* config);

void SLNodeInsertChild(const SLNodeRef parent, const SLNodeRef child,
                       int32_t index);
void SLNodeRemoveChild(const SLNodeRef parent, const SLNodeRef child);
void SLNodeRemoveAllChildren(const SLNodeRef parent);
void SLNodeReset(const SLNodeRef node);
SLNodeRef SLNodeGetChild(const SLNodeRef node, int32_t index);
int32_t SLNodeGetChildCount(const SLNodeRef node);
SLNodeRef SLNodeGetParent(const SLNodeRef node);

void SLNodeFree(const SLNodeRef node);
void SLNodeFreeRecursive(const SLNodeRef node);

bool SLNodeIsDirty(const SLNodeRef node);
void SLNodeMarkDirty(const SLNodeRef node);

// TODO(yuanzhiwen): currently unavailable
// bool SLNodeGetHasNewLayout(const SLNodeRef node);
// void SLNodeSetHasNewLayout(const SLNodeRef node, bool has_new_layout);

bool SLNodeIsRTL(const SLNodeRef node);

// owner_width, owner_height define the containing block for the root node, and
// the percentage for width, height, max-width, max-height, padding, margin will
// refer to this containing block. If width, height are not set, the root node's
// outer box will be limited by the owner_width and owner_height.
void SLNodeCalculateLayout(const SLNodeRef node, float owner_width,
                           float owner_height, SLDirection owner_direction);

void SLNodeSetContext(const SLNodeRef node,
                      StarlightMeasureDelegate* const delegate);

StarlightMeasureDelegate* SLNodeGetContext(const SLNodeRef node);

void SLNodeSetMeasureFunc(const SLNodeRef node,
                          StarlightMeasureDelegate* const delegate);
bool SLNodeHasMeasureFunc(const SLNodeRef node);

// Styles
void SLNodeStyleSetDirection(const SLNodeRef node, SLDirection type);
void SLNodeStyleSetFlexDirection(const SLNodeRef node, SLFlexDirection value);

// alignment
void SLNodeStyleSetJustifyContent(const SLNodeRef node, SLJustifyContent value);
void SLNodeStyleSetAlignContent(const SLNodeRef node, SLAlignContent value);
void SLNodeStyleSetAlignItems(const SLNodeRef node, SLFlexAlign value);
void SLNodeStyleSetAlignSelf(const SLNodeRef node, SLFlexAlign value);

// absoulte, relative
void SLNodeStyleSetPositionType(const SLNodeRef node, SLPositionType value);

void SLNodeStyleSetFlexWrap(const SLNodeRef node, SLFlexWrap value);

void SLNodeStyleSetDisplay(const SLNodeRef node, SLDisplay value);

void SLNodeStyleSetBoxSizing(const SLNodeRef node, SLBoxSizing value);

void SLNodeStyleSetAspectRatio(const SLNodeRef node, float value);

void SLNodeStyleSetOrder(const SLNodeRef node, int32_t value);

void SLNodeStyleSetFlex(const SLNodeRef node, float value);
void SLNodeStyleSetFlexGrow(const SLNodeRef node, float value);
void SLNodeStyleSetFlexShrink(const SLNodeRef node, float value);
void SLNodeStyleSetFlexBasis(const SLNodeRef node, float value);
void SLNodeStyleSetFlexBasisPercent(const SLNodeRef node, float value);
void SLNodeStyleSetFlexBasisAuto(const SLNodeRef node);

// top, bottom, left, right
void SLNodeStyleSetPosition(const SLNodeRef node, SLEdge edge, float position);
void SLNodeStyleSetPositionPercent(const SLNodeRef node, SLEdge edge,
                                   float position);
void SLNodeStyleSetPositionAuto(const SLNodeRef node, SLEdge edge);

// margin
void SLNodeStyleSetMargin(const SLNodeRef node, SLEdge edge, float value);
void SLNodeStyleSetMarginPercent(const SLNodeRef node, SLEdge edge,
                                 float value);
void SLNodeStyleSetMarginAuto(const SLNodeRef node, SLEdge edge);

// padding
void SLNodeStyleSetPadding(const SLNodeRef node, SLEdge edge, float value);
void SLNodeStyleSetPaddingPercent(const SLNodeRef node, SLEdge edge,
                                  float value);

// border width
void SLNodeStyleSetBorder(const SLNodeRef node, SLEdge edge, float value);

// gap
void SLNodeStyleSetGap(const SLNodeRef node, SLGap gutter, float value);
void SLNodeStyleSetGapPercent(const SLNodeRef node, SLGap gutter, float value);

// width properties
void SLNodeStyleSetWidth(const SLNodeRef node, float value);
void SLNodeStyleSetWidthPercent(const SLNodeRef node, float value);
void SLNodeStyleSetWidthAuto(const SLNodeRef node);
void SLNodeStyleSetWidthMaxContent(const SLNodeRef node);
void SLNodeStyleSetWidthFitContent(const SLNodeRef node);
void SLNodeStyleSetMinWidth(const SLNodeRef node, float value);
void SLNodeStyleSetMinWidthPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMaxWidth(const SLNodeRef node, float value);
void SLNodeStyleSetMaxWidthPercent(const SLNodeRef node, float value);

// height properties
void SLNodeStyleSetHeight(const SLNodeRef node, float value);
void SLNodeStyleSetHeightPercent(const SLNodeRef node, float value);
void SLNodeStyleSetHeightAuto(const SLNodeRef node);
void SLNodeStyleSetHeightMaxContent(const SLNodeRef node);
void SLNodeStyleSetHeightFitContent(const SLNodeRef node);
void SLNodeStyleSetMinHeight(const SLNodeRef node, float value);
void SLNodeStyleSetMinHeightPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMaxHeight(const SLNodeRef node, float value);
void SLNodeStyleSetMaxHeightPercent(const SLNodeRef node, float value);

// get style
SLFlexDirection SLNodeStyleGetFlexDirection(const SLNodeRef node);
SLJustifyContent SLNodeStyleGetJustifyContent(const SLNodeRef node);
SLAlignContent SLNodeStyleGetAlignContent(const SLNodeRef node);
SLFlexAlign SLNodeStyleGetAlignItems(const SLNodeRef node);
SLFlexAlign SLNodeStyleGetAlignSelf(const SLNodeRef node);
SLPositionType SLNodeStyleGetPositionType(const SLNodeRef node);
SLFlexWrap SLNodeStyleGetFlexWrap(const SLNodeRef node);
SLDisplay SLNodeStyleGetDisplay(const SLNodeRef node);
SLBoxSizing SLNodeStyleGetBoxSizing(const SLNodeRef node);
float SLNodeStyleGetAspectRatio(const SLNodeRef node);
int32_t SLNodeStyleGetOrder(const SLNodeRef node);
float SLNodeStyleGetFlexGrow(const SLNodeRef node);
float SLNodeStyleGetFlexShrink(const SLNodeRef node);
float SLNodeStyleGetBorder(const SLNodeRef node, SLEdge edge);
struct StarlightValue SLNodeStyleGetFlexBasis(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetPosition(const SLNodeRef node, SLEdge edge);
struct StarlightValue SLNodeStyleGetMargin(const SLNodeRef node, SLEdge edge);
struct StarlightValue SLNodeStyleGetPadding(const SLNodeRef node, SLEdge edge);
struct StarlightValue SLNodeStyleGetGap(const SLNodeRef node, SLGap gutter);
struct StarlightValue SLNodeStyleGetWidth(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetHeight(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetMinWidth(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetMaxWidth(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetMinHeight(const SLNodeRef node);
struct StarlightValue SLNodeStyleGetMaxHeight(const SLNodeRef node);

// layout result
float SLNodeLayoutGetLeft(const SLNodeRef node);
float SLNodeLayoutGetTop(const SLNodeRef node);

// TODO(yuanzhiwen): offset_right, and offset_bottom
// float SLNodeLayoutGetRight(const SLNodeRef node);
// float SLNodeLayoutGetBottom(const SLNodeRef node);

float SLNodeLayoutGetWidth(const SLNodeRef node);
float SLNodeLayoutGetHeight(const SLNodeRef node);
float SLNodeLayoutGetMargin(const SLNodeRef node, SLEdge edge);
float SLNodeLayoutGetPadding(const SLNodeRef node, SLEdge edge);
float SLNodeLayoutGetBorder(const SLNodeRef node, SLEdge edge);

#ifdef __cplusplus
}
#endif

#endif  // CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_
