// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIIntersectionObserverUnitTest.h"
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUIOwner.h>

@implementation LynxUIIntersectionObserverUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testObserve {
  LynxUIIntersectionObserverManager *observerManager =
      [[LynxUIIntersectionObserverManager alloc] init];
  LynxView *lynxView = [LynxView new];
  LynxUIOwner *uiOwner = [[LynxUIOwner alloc] initWithContainerView:lynxView
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  [uiOwner createUIWithSign:0
                    tagName:@"page"
                   eventSet:nil
              lepusEventSet:nil
                      props:nil
                  nodeIndex:0
         gestureDetectorSet:nil];
  [uiOwner.rootUI setIdSelector:@"box"];
  observerManager.uiOwner = uiOwner;
  LynxUIIntersectionObserver *mockObserver =
      OCMPartialMock([[LynxUIIntersectionObserver alloc] initWithManager:observerManager
                                                              observerId:0
                                                             componentId:@"0"
                                                                 options:nil]);
  NSMutableArray *array = OCMPartialMock([NSMutableArray new]);
  mockObserver.observationTargets = array;

  [mockObserver observe:@"#box" callbackId:0];
  XCTAssertTrue([array count] == 1);
  [mockObserver disconnect];
  [mockObserver observe:@"0" callbackId:0];
  XCTAssertTrue([array count] == 1);
}

@end
