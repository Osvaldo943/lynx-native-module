// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBackgroundManager.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIView (Lynx)

@property(nonatomic, copy) NSNumber* lynxSign;

@property(nonatomic, readwrite) BOOL lynxClickable;

@property(nonatomic, readwrite) BOOL lynxEnableTapGestureSimultaneously;

@property(nonatomic, readwrite, strong, nullable) LynxBackgroundSubLayer* backgroundLayer;

@property(nonatomic, readwrite, strong, nullable) LynxBorderLayer* borderLayer;

@end

NS_ASSUME_NONNULL_END
