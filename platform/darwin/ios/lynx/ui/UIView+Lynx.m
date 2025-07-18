// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/UIView+Lynx.h>
#import <objc/runtime.h>

@implementation UIView (Lynx)

- (void)setBackgroundLayer:(LynxBackgroundSubLayer *)backgroundLayer {
  objc_setAssociatedObject(self, @selector(backgroundLayer), backgroundLayer,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxBackgroundSubLayer *)backgroundLayer {
  return objc_getAssociatedObject(self, @selector(backgroundLayer));
}

- (void)setBorderLayer:(LynxBorderLayer *)borderLayer {
  objc_setAssociatedObject(self, @selector(borderLayer), borderLayer,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxBorderLayer *)borderLayer {
  return objc_getAssociatedObject(self, @selector(borderLayer));
}

- (NSNumber *)lynxSign {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setLynxSign:(NSNumber *)sign {
  objc_setAssociatedObject(self, @selector(lynxSign), sign, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)lynxClickable {
  NSNumber *number = objc_getAssociatedObject(self, _cmd);
  return [number boolValue];
}

- (void)setLynxClickable:(BOOL)clickable {
  objc_setAssociatedObject(self, @selector(lynxClickable), [NSNumber numberWithBool:clickable],
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)lynxEnableTapGestureSimultaneously {
  NSNumber *number = objc_getAssociatedObject(self, _cmd);
  return [number boolValue];
}

- (void)setLynxEnableTapGestureSimultaneously:(BOOL)enable {
  objc_setAssociatedObject(self, @selector(lynxEnableTapGestureSimultaneously),
                           [NSNumber numberWithBool:enable], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
