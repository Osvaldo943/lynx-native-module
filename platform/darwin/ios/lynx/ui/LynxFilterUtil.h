// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxCSSType.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxFilterUtil : NSObject

+ (id)getFilterWithType:(LynxFilterType)type filterAmount:(CGFloat)filter_amount;

@end

NS_ASSUME_NONNULL_END
