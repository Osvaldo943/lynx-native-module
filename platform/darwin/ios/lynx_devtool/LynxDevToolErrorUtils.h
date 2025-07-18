// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxError.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevToolErrorUtils : NSObject

+ (NSString*)getKeyMessage:(LynxError*)error;
+ (NSInteger)intValueFromErrorLevelString:(NSString*)levelStr;

@end

NS_ASSUME_NONNULL_END
