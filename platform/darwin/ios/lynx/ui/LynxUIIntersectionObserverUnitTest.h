
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIIntersectionObserver.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxUIIntersectionObserver ()
@property(nonatomic, strong) NSMutableArray<LynxUIObservationTarget*>* observationTargets;
@end

@interface LynxUIIntersectionObserverUnitTest : XCTestCase
@end
