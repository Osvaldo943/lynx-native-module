// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxWeakProxy.h>

@implementation LynxWeakProxy

- (instancetype)initWithTarget:(id)target {
  _target = target;
  return self;
}

+ (instancetype)proxyWithTarget:(id)target {
  return [[self alloc] initWithTarget:target];
}

- (id)forwardingTargetForSelector:(SEL)selector {
  return _target;
}

- (void)forwardInvocation:(NSInvocation *)invocation {
  SEL sel = invocation.selector;
  if ([_target respondsToSelector:sel]) {
    [invocation invokeWithTarget:_target];
  }
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)sel {
  if (_target) {
    return [_target methodSignatureForSelector:sel];
  }
  return [NSObject instanceMethodSignatureForSelector:@selector(init)];
}

- (BOOL)respondsToSelector:(SEL)aSelector {
  return [_target respondsToSelector:aSelector];
}

- (BOOL)isEqual:(id)object {
  return [_target isEqual:object];
}

- (NSUInteger)hash {
  return [_target hash];
}

- (Class)superclass {
  return [_target superclass];
}

- (Class)class {
  return [_target class];
}

- (BOOL)isKindOfClass:(Class)aClass {
  return [_target isKindOfClass:aClass];
}

- (BOOL)isMemberOfClass:(Class)aClass {
  return [_target isMemberOfClass:aClass];
}

- (BOOL)conformsToProtocol:(Protocol *)aProtocol {
  return [_target conformsToProtocol:aProtocol];
}

- (BOOL)isProxy {
  return YES;
}

- (NSString *)description {
  return [_target description];
}

- (NSString *)debugDescription {
  return [_target debugDescription];
}

@end
