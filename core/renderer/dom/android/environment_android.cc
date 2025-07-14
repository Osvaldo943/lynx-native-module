// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/android/environment_android.h"

#include "platform/android/lynx_android/src/main/jni/gen/EnvUtils_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/EnvUtils_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForEnvUtils(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace base {
namespace android {

std::string EnvironmentAndroid::GetCacheDir() {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> dir =
      Java_EnvUtils_getCacheDir(env);
  const char* str = env->GetStringUTFChars(dir.Get(), JNI_FALSE);
  std::string path;
  if (str) {
    path = std::string(str);
  }
  env->ReleaseStringUTFChars(dir.Get(), str);
  return path;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
