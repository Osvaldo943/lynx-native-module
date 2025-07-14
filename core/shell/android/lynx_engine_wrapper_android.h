// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_LYNX_ENGINE_WRAPPER_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_ENGINE_WRAPPER_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
namespace lynx {
namespace shell {

class LynxEngineWrapperAndroid {
 public:
  static void RegisterJni(JNIEnv* env);
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_ENGINE_WRAPPER_ANDROID_H_
