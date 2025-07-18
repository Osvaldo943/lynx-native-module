// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxInspectorOwner.h>

#include <string>

NS_ASSUME_NONNULL_BEGIN

@interface LynxInspectorOwner ()

- (void)sendResponse:(std::string)response;

@end

NS_ASSUME_NONNULL_END
