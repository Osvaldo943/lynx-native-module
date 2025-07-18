// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxCustomMeasureDelegate+Internal.h>
#import <Lynx/LynxCustomMeasureDelegate.h>
#import <Lynx/LynxCustomMeasureShadowNode.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUnitUtils.h>

@interface LynxCustomMeasureShadowNode () <LynxCustomMeasureDelegate>

@end

@implementation LynxCustomMeasureShadowNode

LYNX_PROP_SETTER("custom-layout", customLayout, BOOL) { _hasCustomLayout = value; }

- (BOOL)supportInlineView {
  return YES;
}

- (void)adoptNativeLayoutNode:(int64_t)ptr {
  if (_hasCustomLayout) {
    [self setCustomMeasureDelegate:self];
  }
  [super adoptNativeLayoutNode:ptr];
}

- (MeasureResult)measureNativeLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)ctx {
  return [self.layoutNodeManager measureWithSign:self.sign MeasureParam:param MeasureContext:ctx];
}

- (void)alignNativeLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context {
  [self.layoutNodeManager alignWithSign:self.sign AlignParam:param AlignContext:context];
}

- (MeasureResult)customMeasureLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)ctx {
  return (MeasureResult){CGSizeZero, 0.f};
}

- (void)customAlignLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context {
}

#pragma mark - LynxCustomMeasureDelegate

- (void)alignWithAlignParam:(nonnull AlignParam *)param
               AlignContext:(nonnull AlignContext *)context {
  [self customAlignLayoutNode:param alignContext:context];
}

- (MeasureResult)measureWithMeasureParam:(nonnull MeasureParam *)param
                          MeasureContext:(nullable MeasureContext *)context {
  MeasureResult result = [self customMeasureLayoutNode:param measureContext:context];
  return result;
}

- (CGFloat)toPtWithUnitValue:(NSString *)unitValue fontSize:(CGFloat)fontSize {
  LynxUI *rootUI = (LynxUI *)self.uiOwner.uiContext.rootUI;
  return [LynxUnitUtils toPtWithScreenMetrics:self.uiOwner.uiContext.screenMetrics
                                    unitValue:unitValue
                                 rootFontSize:rootUI.fontSize
                                  curFontSize:fontSize
                                    rootWidth:CGRectGetWidth(rootUI.frame)
                                   rootHeight:CGRectGetHeight(rootUI.frame)
                                withDefaultPt:0];
}

@end
