// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderEnv.h>

@interface LynxRecorderEnv ()

@end

@implementation LynxRecorderEnv

+ (instancetype)sharedInstance {
  static LynxRecorderEnv *sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  if (self = [super init]) {
    self.lynxRecorderUrlPrefix = @"file://lynxrecorder?";
    self.lynxRecorderUrlSchema = @"file";
    self.lynxRecorderUrlHost = @"lynxrecorder";
  }
  return self;
}
@end
