// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_
#define CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_

#include <jni.h>

#include <string>

namespace lynx {
namespace base {
namespace android {
class EnvironmentAndroid {
 public:
  // TODO(liyanbo.monster): If this method is called from multiple places,
  // thread-safe caching logic should be added to ensure performance. Currently,
  // it is only called in js_cache_manager, so there is no need to worry about
  // it.
  static std::string GetCacheDir();
};

}  // namespace android
}  // namespace base
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_
