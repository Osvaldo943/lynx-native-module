// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderReplayDataModule.h>

@interface LynxRecorderReplayDataModule ()
@property NSArray *functionCall;
@property NSDictionary *callbackData;
@property NSArray *jsbIgnoredInfo;
@property NSDictionary *jsbSettings;
@end

@implementation LynxRecorderReplayDataModule

+ (NSString *)name {
  return @"LynxRecorderReplayDataModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"getData" : NSStringFromSelector(@selector(getData:)),
  };
}

- (id)initWithParam:(id)param {
  if (self = [super init]) {
    if ([param conformsToProtocol:@protocol(LynxRecorderReplayDataProvider)]) {
      id<LynxRecorderReplayDataProvider> provider = (id<LynxRecorderReplayDataProvider>)param;
      _functionCall = [provider getFunctionCall];
      _callbackData = [provider getCallbackData];
      _jsbSettings = [provider getJsbSettings];
      _jsbIgnoredInfo = [provider getJSbIgnoredInfo];
    }
  }
  return self;
}

- (void)getData:(LynxCallbackBlock)callback {
  NSMutableDictionary *json = [NSMutableDictionary new];
  [json setObject:[self getRecordData] forKey:@"RecordData"];
  [json setObject:[self getJsbSettings] forKey:@"JsbSettings"];
  [json setObject:[self getJsbIgnoredInfo] forKey:@"JsbIgnoredInfo"];
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:json options:0 error:0];
  NSString *strData = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  callback(strData);
}

- (NSString *)getRecordData {
  NSMutableDictionary<NSString *, NSMutableArray *> *json = [NSMutableDictionary new];
  for (int index = 0; index < [_functionCall count]; index++) {
    NSDictionary *funcInvoke = _functionCall[index];
    NSString *moduleName = [funcInvoke objectForKey:@"Module Name"];
    if (![json objectForKey:moduleName]) {
      [json setObject:[NSMutableArray new] forKey:moduleName];
    }

    NSMutableDictionary *methodLookUp = [NSMutableDictionary new];

    NSString *methodName = [funcInvoke objectForKey:@"Method Name"];

    long requestTime = [[funcInvoke objectForKey:@"Record Time"] doubleValue] * 1000;

    if ([funcInvoke objectForKey:@"RecordMillisecond"]) {
      requestTime = [[funcInvoke objectForKey:@"RecordMillisecond"] doubleValue];
    }

    NSMutableDictionary *params = [funcInvoke objectForKey:@"Params"];

    NSArray *callbackIDs = [params objectForKey:@"callback"];

    NSMutableArray *callbackReturnValues = [NSMutableArray new];

    NSString *functionInvokeLabel = @"";
    for (int i = 0; i < callbackIDs.count; i++) {
      NSDictionary *callbackInfo = [_callbackData objectForKey:callbackIDs[i]];
      if (callbackInfo != nil) {
        long responseTime = [[callbackInfo objectForKey:@"Record Time"] doubleValue] * 1000;
        if ([callbackInfo objectForKey:@"RecordMillisecond"]) {
          responseTime = [[callbackInfo objectForKey:@"RecordMillisecond"] doubleValue];
        }
        NSDictionary *callbackKernel = @{
          @"Value" : [callbackInfo objectForKey:@"Params"],
          @"Delay" : @(responseTime - requestTime)
        };
        [callbackReturnValues addObject:callbackKernel];
      }
      functionInvokeLabel = [functionInvokeLabel stringByAppendingFormat:@"%@_", callbackIDs[i]];
    }

    [methodLookUp setObject:methodName forKey:@"Method Name"];
    [methodLookUp setObject:params forKey:@"Params"];
    [methodLookUp setObject:callbackReturnValues forKey:@"Callback"];
    [methodLookUp setObject:functionInvokeLabel forKey:@"Label"];

    if ([funcInvoke objectForKey:@"SyncAttributes"]) {
      [methodLookUp setObject:[funcInvoke objectForKey:@"SyncAttributes"] forKey:@"SyncAttributes"];
    }

    [[json objectForKey:moduleName] addObject:methodLookUp];
  }
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:json options:0 error:0];
  NSString *strData = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  return strData;
}

- (NSString *)getJsbIgnoredInfo {
  if (_jsbIgnoredInfo != nil) {
    NSData *reData = [NSJSONSerialization dataWithJSONObject:_jsbIgnoredInfo options:0 error:0];
    NSString *infoData = [[NSString alloc] initWithData:reData encoding:NSUTF8StringEncoding];
    return infoData;
  } else {
    return @"[]";
  }
}

- (NSString *)getJsbSettings {
  if (_jsbSettings != nil) {
    NSData *reData = [NSJSONSerialization dataWithJSONObject:_jsbSettings options:0 error:0];
    NSString *settingData = [[NSString alloc] initWithData:reData encoding:NSUTF8StringEncoding];
    return settingData;
  } else {
    return @"{}";
  }
}

@end
