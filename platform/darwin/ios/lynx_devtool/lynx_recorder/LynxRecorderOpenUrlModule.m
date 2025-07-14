// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderOpenUrlModule.h>
#import <LynxDevtool/LynxRecorderPageManager.h>

@implementation LynxRecorderOpenUrlModule

+ (NSString *)name {
  return @"LynxRecorderOpenUrlModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"openSchema" : NSStringFromSelector(@selector(openSchema:)),
  };
}

- (void)openSchema:(NSDictionary *)params {
  [[LynxRecorderPageManager sharedInstance] replayPageFromOpenSchema:params];
}

@end
