// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxRecorderActionCallback;
@interface LynxRecorderEntranceViewController : UIViewController

@property(nonatomic, copy) NSString *url;

+ (BOOL)entranceLynxRecorderView:(id<LynxRecorderActionCallback>)actionCallback;

+ (BOOL)enableLoadLocalRecordFile:(NSString *)url;

+ (NSData *)localRecordFileData:(NSString *)url;

@end

NS_ASSUME_NONNULL_END
