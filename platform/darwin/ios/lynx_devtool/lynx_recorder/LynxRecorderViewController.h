// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxGroup.h>
#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN

@protocol LynxRecorderActionCallback;
@interface LynxRecorderViewController : UIViewController
@property(nonatomic, copy) NSString* url;
@property(nonatomic, copy) NSString* sourceUrl;
- (id)initWithGroup:(nullable LynxGroup*)group;
@property Boolean fullScreen;
@property BOOL hasBeenPop;
@property NSString* pageName;
@property(nonatomic, copy) void (^endTestBenchBlock)(void);
- (void)registerLynxRecorderActionCallback:(id<LynxRecorderActionCallback>)callback;
- (LynxGroup*)getLynxGroup;
- (void)setLynxGroup:(LynxGroup*)group;
@end

NS_ASSUME_NONNULL_END
