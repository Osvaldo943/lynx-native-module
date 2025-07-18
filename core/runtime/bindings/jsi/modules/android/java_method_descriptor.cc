// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/java_method_descriptor.h"

#include "platform/android/lynx_android/src/main/jni/gen/MethodDescriptor_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/MethodDescriptor_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForMethodDescriptor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace piper {

std::string JavaMethodDescriptor::getName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> obj =
      Java_MethodDescriptor_getName(env, wrapper_.Get());
  const char* str = env->GetStringUTFChars(obj.Get(), JNI_FALSE);
  std::string ret(str ? str : "");
  env->ReleaseStringUTFChars(obj.Get(), str);
  return ret;
}

std::string JavaMethodDescriptor::getSignature() {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> obj =
      Java_MethodDescriptor_getSignature(env, wrapper_.Get());
  const char* str = env->GetStringUTFChars(obj.Get(), JNI_FALSE);
  std::string ret(str ? str : "");
  env->ReleaseStringUTFChars(obj.Get(), str);
  return ret;
}

lynx::base::android::ScopedLocalJavaRef<jobject>
JavaMethodDescriptor::getMethod() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_MethodDescriptor_getMethod(env, wrapper_.Get());
}

}  // namespace piper
}  // namespace lynx
