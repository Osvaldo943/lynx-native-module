// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxRootUI.h>
#import "LynxEngineProxy.h"
#import "LynxTemplateData+Converter.h"
#import "LynxUIIntersectionObserver.h"

using namespace lynx::tasm;
using namespace lynx::lepus;

@implementation LynxEventEmitter {
  LynxEngineProxy* _engineProxy;
  NSMutableArray* eventObservers_;
  onLynxEvent eventReporter_;
  dispatch_block_t intersectionObserver_;
  int64_t eventID_;
}

- (instancetype)initWithLynxEngineProxy:(LynxEngineProxy*)engineProxy {
  self = [super init];
  if (self) {
    eventObservers_ = [[NSMutableArray alloc] init];
    _engineProxy = engineProxy;
  }
  return self;
}

- (void)setEventReporterBlock:(onLynxEvent)eventReporter {
  eventReporter_ = eventReporter;
}

- (void)setIntersectionObserverBlock:(dispatch_block_t)intersectionObserver {
  intersectionObserver_ = intersectionObserver;
}

- (BOOL)dispatchTouchEvent:(LynxTouchEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchTouchEvent event: %@ failed since engineProxy is nil", event.eventName);
    return NO;
  }
  if ([self onLynxEvent:event]) {
    return YES;
  }
  id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
  if ([target parentLynxPageUI] || [target childrenLynxPageUI]) {
    if ([target parentLynxPageUI] == nil) {
      eventID_ = (int64_t)(event.timestamp * 1000);
    }
    event.eventID = eventID_;
    [target setEventID:eventID_];
    [self startEventGenerate:event];

    if ([target childrenLynxPageUI][[NSString stringWithFormat:@"%p", target]] == nil) {
      [target.rootLynxPageUI startEventCapture:eventID_];
    }
  } else {
    [_engineProxy sendSyncTouchEvent:event];
  }
  return NO;
}

- (BOOL)onLynxEvent:(LynxEvent*)detail {
  if (eventReporter_ == nil) {
    _LogE(@"onLynxEvent event: %@ failed since eventReporter is nil", detail.eventName);
    return NO;
  }
  return eventReporter_(detail);
}

- (void)dispatchMultiTouchEvent:(LynxTouchEvent*)event {
  if (_engineProxy == nil) {
    LLogError(@"dispatchMultiTouchEvent event: %@ failed since engineProxy is nil",
              event.eventName);
    return;
  }
  if ([self onLynxEvent:event]) {
    return;
  }
  id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
  if ([target parentLynxPageUI] || [target childrenLynxPageUI]) {
    id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
    if ([target parentLynxPageUI] == nil) {
      eventID_ = (int64_t)(event.timestamp * 1000);
    }
    event.eventID = eventID_;
    [target setEventID:eventID_];
    [self startEventGenerate:event];

    if ([target childrenLynxPageUI][[NSString stringWithFormat:@"%p", target]] == nil) {
      [target.rootLynxPageUI startEventCapture:eventID_];
    }
  } else {
    [_engineProxy sendSyncMultiTouchEvent:event];
  }
}

- (void)dispatchCustomEvent:(LynxCustomEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchCustomEvent event: %@ failed since engineProxy is nil", event.eventName);
    return;
  }
  if ([self onLynxEvent:event]) {
    return;
  }
  [event addDetailKey:@"timestamp" value:@((int64_t)(event.timestamp * 1000))];
  [_engineProxy sendCustomEvent:event];
  [self notifyEventObservers:LynxEventTypeCustomEvent event:event];
}

// dispatch gesture event to template render
- (void)dispatchGestureEvent:(int)gestureId event:(LynxCustomEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchGestureEvent event: %@ failed since engineProxy is nil", event.eventName);
    return;
  }
  [_engineProxy sendGestureEvent:gestureId event:event];
}

- (void)sendCustomEvent:(LynxCustomEvent*)event {
  [self dispatchCustomEvent:event];
}

- (void)onPseudoStatusChanged:(int32_t)tag
                fromPreStatus:(int32_t)preStatus
              toCurrentStatus:(int32_t)currentStatus {
  if (_engineProxy == nil) {
    _LogE(@"onPseudoStatusChanged id: %d failed since engineProxy is nil", tag);
    return;
  }
  if (preStatus == currentStatus) {
    return;
  }
  [_engineProxy onPseudoStatusChanged:tag fromPreStatus:preStatus toCurrentStatus:currentStatus];
}

- (void)dispatchLayoutEvent {
  [self notifyEventObservers:LynxEventTypeLayoutEvent event:nil];
}

- (void)addObserver:(id<LynxEventObserver>)observer {
  if (![eventObservers_ containsObject:observer]) {
    [eventObservers_ addObject:observer];
  }
}

- (void)removeObserver:(id<LynxEventObserver>)observer {
  if ([eventObservers_ containsObject:observer]) {
    [eventObservers_ removeObject:observer];
  }
}

- (void)notifyEventObservers:(LynxInnerEventType)type event:(LynxEvent*)event {
  [eventObservers_
      enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        // if use new IntersectionObserver, don't notify observer here.
        if ([obj isKindOfClass:[LynxUIIntersectionObserverManager class]] &&
            ((LynxUIIntersectionObserverManager*)obj).enableNewIntersectionObserver) {
          return;
        }
        [(id<LynxEventObserver>(obj)) onLynxEvent:type event:event];
      }];
}

- (void)notifyIntersectionObserver {
  if (strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL),
             dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {
    // if on main thread, exec block
    intersectionObserver_();
  } else {
    // if not on main thread, post to main thread
    dispatch_async(dispatch_get_main_queue(), intersectionObserver_);
  }
}

- (void)startEventGenerate:(LynxEvent*)event {
  [_engineProxy startEventGenerate:(LynxTouchEvent*)event];
}

- (void)setEventID:(int64_t)eventID {
  eventID_ = eventID;
}

- (void)startEventCapture:(int64_t)eventID {
  [_engineProxy startEventCapture:eventID];
}

- (void)startEventBubble:(int64_t)eventID {
  [_engineProxy startEventBubble:eventID];
}

- (void)startEventFire:(BOOL)isStop withEventID:(int64_t)eventID {
  [_engineProxy startEventFire:isStop withEventID:eventID];
}

@end
