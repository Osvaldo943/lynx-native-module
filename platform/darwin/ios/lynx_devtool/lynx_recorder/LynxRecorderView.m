// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderActionManager.h>
#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <LynxDevtool/LynxRecorderReplayStateView.h>
#import <LynxDevtool/LynxRecorderView.h>

@interface LynxRecorderView ()

@property LynxRecorderActionManager *actionManager;
@property __weak UIScrollView *scrollContainer;

@end

@implementation LynxRecorderView

- (void)registerLynxRecorderActionCallback:(id<LynxRecorderActionCallback>)callback {
  [_actionManager registerLynxRecorderActionCallback:callback];
}

- (id)init {
  if (self = [super init]) {
    _actionManager = [[LynxRecorderActionManager alloc] init];
  }
  return self;
}

- (void)reload {
  if (_actionManager) {
    [_actionManager reload];
  }
}

- (void)updateScrollContainerSize {
  CGFloat width = self.frame.size.width + self.frame.origin.x;
  CGFloat height = self.frame.size.height + self.frame.origin.y;

  CGSize size = _scrollContainer.contentSize;
  if (size.width < width) {
    size.width = width;
  }

  if (size.height < height) {
    size.height = height;
  }
  _scrollContainer.contentSize = size;
}

- (void)loadPageWithPoint:(NSString *)url
                    point:(CGPoint)point
          scrollContainer:(nonnull UIScrollView *)scrollContainer {
  _scrollContainer = scrollContainer;
  CGRect frame = self.frame;
  frame.origin = point;
  self.frame = frame;
  [_actionManager startWithUrl:url
                        inView:self
                    withOrigin:CGPointMake(0, 0)
                  replayConfig:[[LynxRecorderReplayConfig alloc] initWithProductUrl:url]];
}

@end
