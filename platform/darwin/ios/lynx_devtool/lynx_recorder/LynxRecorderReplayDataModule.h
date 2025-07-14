// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxModule.h>

NS_ASSUME_NONNULL_BEGIN
@protocol LynxRecorderReplayDataProvider <NSObject>
@required
- (NSArray *)getFunctionCall;
@required
- (NSDictionary *)getCallbackData;
@required
- (NSArray *)getJSbIgnoredInfo;
@required
- (NSDictionary *)getJsbSettings;
@end

@interface LynxRecorderReplayDataModule : NSObject <LynxModule>
- (id)initWithParam:(id)param;
@end

NS_ASSUME_NONNULL_END
