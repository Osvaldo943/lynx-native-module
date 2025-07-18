// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxClassAliasDefines.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxColorUtils : NSObject

+ (nullable COLOR_CLASS*)convertNSStringToUIColor:(NSString*)value;

@end

NS_ASSUME_NONNULL_END
