// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxRecorderEnv : NSObject
+ (instancetype)sharedInstance;
@property NSString* lynxRecorderUrlPrefix;
@property NSString* lynxRecorderUrlSchema;
@property NSString* lynxRecorderUrlHost;
@end

NS_ASSUME_NONNULL_END
