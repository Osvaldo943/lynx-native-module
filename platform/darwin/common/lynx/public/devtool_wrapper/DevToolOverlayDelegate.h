// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/OverlayService.h>

@interface DevToolOverlayDelegate : NSObject

+ (instancetype)sharedInstance;

- (NSArray<NSNumber *> *)getAllVisibleOverlaySign;
- (void)initWithService:(id<OverlayService>)service;

@end
