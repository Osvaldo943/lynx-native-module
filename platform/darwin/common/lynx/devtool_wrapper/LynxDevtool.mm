// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDevtool.h>

#import <AudioToolbox/AudioToolbox.h>

#import <Lynx/LynxBaseInspectorOwner.h>
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxDevtool+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <Lynx/LynxPageReloadHelper.h>
#import <Lynx/LynxTraceEventDef.h>

#import <Lynx/LynxError.h>
#import <Lynx/LynxLogBoxProtocol.h>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceDevToolProtocol.h>

#pragma mark - LynxDevtool
@implementation LynxDevtool {
  __weak LynxView *_lynxView;
  id<LynxLogBoxProtocol> _logbox;

  LynxPageReloadHelper *_reloader;
  id<LynxViewStateListener> _lynxViewStateListener;
}

- (nonnull instancetype)initWithLynxView:(LynxView *)view debuggable:(BOOL)debuggable {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_INIT_WITH_LYNX_VIEW);
  _lynxView = view;

  if (LynxEnv.sharedInstance.lynxDebugEnabled) {
    id<LynxServiceDevToolProtocol> devtoolService = LynxService(LynxServiceDevToolProtocol);
    if (!devtoolService) {
      _LogW(@"LynxServiceDevToolProtocol instance not found");
    }
    if ((LynxEnv.sharedInstance.devtoolEnabled ||
         (LynxEnv.sharedInstance.devtoolEnabledForDebuggableView && debuggable)) &&
        devtoolService) {
      _owner = [devtoolService createInspectorOwnerWithLynxView:view];
    } else {
      _owner = nil;
    }

    if (LynxEnv.sharedInstance.logBoxEnabled && devtoolService) {
      _logbox = [devtoolService createLogBoxWithLynxView:view];
      [_logbox setLynxDevTool:self];
    } else {
      _logbox = nil;
    }
  }

  if (_owner != nil) {
    _reloader = [[LynxPageReloadHelper alloc] initWithLynxView:view];
  } else {
    _reloader = nil;
  }

  return self;
}

- (void)registerModule:(LynxTemplateRender *)render {
  id<LynxServiceDevToolProtocol> devtoolService = LynxService(LynxServiceDevToolProtocol);
  if (!devtoolService) {
    _LogW(@"LynxServiceDevToolProtocol instance not found");
  }
  Class moduleClass = [devtoolService devtoolSetModuleClass];
  if ([moduleClass conformsToProtocol:@protocol(LynxContextModule)]) {
    [render registerModule:moduleClass param:nil];
  } else {
    _LogE(@"failed to register LynxDevToolSetModule!");
  }
  Class socketModuleClass = [devtoolService devtoolWebSocketModuleClass];
  if ([socketModuleClass conformsToProtocol:@protocol(LynxContextModule)]) {
    [render registerModule:socketModuleClass param:nil];
  } else {
    _LogE(@"failed to register LynxWebSocketModule!");
  }
  Class trailModuleClass = [devtoolService devtoolTrailModuleClass];
  if ([trailModuleClass conformsToProtocol:@protocol(LynxContextModule)]) {
    [render registerModule:trailModuleClass param:nil];
  } else {
    _LogE(@"failed to register LynxTrailModule!");
  }
}

- (void)onLoadFromLocalFile:(NSData *)tem
                    withURL:(NSString *)url
                   initData:(LynxTemplateData *)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_ON_LOAD_FROM_LOCAL_FILE, "url", [url UTF8String]);
  if (_reloader != nil) {
    [_reloader loadFromLocalFile:tem withURL:url initData:data];
  }
  [_logbox onLynxViewReload];
}

- (void)onLoadFromURL:(NSString *)url
             initData:(LynxTemplateData *)data
              postURL:(NSString *)postUrl {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_ON_LOAD_FROM_URL, "url", [url UTF8String]);
  if (_reloader != nil) {
    [_reloader loadFromURL:url initData:data];
  }
  [_logbox onLynxViewReload];
}

- (void)attachDebugBridge:(NSString *)url {
  [_owner attachDebugBridge:url];
}

- (void)onLoadFromBundle:(LynxTemplateBundle *)bundle
                 withURL:(NSString *)url
                initData:(LynxTemplateData *)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_ON_LOAD_FROM_BUNDLE, "url", [url UTF8String]);
  if (_reloader != nil) {
    [_reloader loadFromBundle:bundle withURL:url initData:data];
  }
  [_logbox onLynxViewReload];
}

- (void)onUpdateDataWithTemplateData:(LynxTemplateData *)data {
  if (_reloader != nil) {
    [_reloader updateDataWithTemplateData:data];
  }
}

- (void)onResetDataWithTemplateData:(LynxTemplateData *)data {
  if (_reloader != nil) {
    [_reloader resetDataWithTemplateData:data];
  }
}

- (void)onStandaloneRuntimeLoadFromURL:(NSString *)url {
  [_reloader loadFromURL:url initData:[[LynxTemplateData alloc] initWithDictionary:@{}]];
}

#if TARGET_OS_IOS
- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime *)runtime
                   groupThreadName:(NSString *)groupThreadName {
  [_owner onBackgroundRuntimeCreated:runtime groupThreadName:groupThreadName];
}
#endif

- (void)onTemplateAssemblerCreated:(intptr_t)ptr {
  if (_owner != nil) {
    [_owner onTemplateAssemblerCreated:ptr];
  }
}

- (void)onEnterForeground {
  if (_owner != nil) {
    [_owner continueCasting];
  }
  if (_lynxViewStateListener) {
    [_lynxViewStateListener onEnterForeground];
  }
}

- (void)onEnterBackground {
  if (_owner != nil) {
    [_owner pauseCasting];
  }
  if (_lynxViewStateListener) {
    [_lynxViewStateListener onEnterBackground];
  }
}

- (void)onMovedToWindow {
  if ([_owner respondsToSelector:@selector(onMovedToWindow)]) {
    [_owner performSelector:@selector(onMovedToWindow)];
  }
  if (_lynxViewStateListener) {
    [_lynxViewStateListener onMovedToWindow];
  }
  [_logbox onMovedToWindow];
}

- (void)onLoadFinished {
  if (_owner != nil) {
    [_owner onLoadFinished];
  }
  if (_lynxViewStateListener) {
    [_lynxViewStateListener onLoadFinished];
  }
}

- (void)handleLongPress {
  if (_owner != nil) {
    [_owner handleLongPress];
  }
}

- (void)showErrorMessage:(LynxError *)error {
  [_logbox showLogMessage:error];
  [_owner showErrorMessageOnConsole:error];
}

- (void)attachLynxView:(LynxView *)lynxView {
  _lynxView = lynxView;
  if (_owner != nil) {
    [_owner attach:lynxView];
  }
  if (_reloader != nil) {
    [_reloader attachLynxView:lynxView];
  }
  [_logbox attachLynxView:lynxView];
}

- (void)dealloc {
  if (_lynxViewStateListener) {
    [_lynxViewStateListener onDestroy];
  }
  [_logbox destroy];
}

- (void)onPageUpdate {
  if (_owner != nil) {
    [_owner onPageUpdate];
  }
}

- (void)downloadResource:(NSString *)url callback:(LynxResourceLoadBlock)callback {
  if (_owner != nil) {
    [_owner downloadResource:url callback:callback];
  }
}

// Use TARGET_OS_IOS rather than OS_IOS to stay consistent with the header file
#if TARGET_OS_IOS
- (void)attachLynxUIOwner:(nullable LynxUIOwner *)uiOwner {
  if (_owner != nil) {
    [_owner attachLynxUIOwnerToAgent:uiOwner];
    [_owner setReloadHelper:_reloader];
  }
}
#endif

- (void)onTemplateLoadSuccess:(nullable NSData *)tem {
  if (_reloader != nil) {
    [_reloader onTemplateLoadSuccess:tem];
  }
}

- (void)onGlobalPropsUpdated:(LynxTemplateData *)props {
  if (_owner != nil) {
    [_owner onGlobalPropsUpdated:props];
  }
}

- (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data {
  if (_owner != nil) {
    [_owner onPerfMetricsEvent:eventName withData:data];
  }
}

- (NSString *)debugInfoUrl:(NSString *)filename {
  return [_owner debugInfoUrl:filename];
}

- (void)onReceiveMessageEvent:(NSDictionary *)event {
  if (_owner != nil) {
    [_owner onReceiveMessageEvent:event];
  }
}

- (void)setDispatchMessageEventBlock:(void (^)(NSDictionary *))block {
  if (_owner != nil) {
    [_owner setDispatchMessageEventBlock:block];
  }
}

@end
