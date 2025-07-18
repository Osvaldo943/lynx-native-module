// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxShadowNode.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIBaseInputShadowNode :  LynxShadowNode <LynxMeasureDelegate>

@property (atomic, assign) CGSize uiSize;

@property (nonatomic, assign) CGFloat widthForMeasure;

@end

NS_ASSUME_NONNULL_END
