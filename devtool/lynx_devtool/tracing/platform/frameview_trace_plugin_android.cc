// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "devtool/lynx_devtool/tracing/platform/frameview_trace_plugin_android.h"

#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/FrameViewTrace_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/FrameViewTrace_register_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForFrameViewTrace(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx

static jlong CreateFrameViewTrace(JNIEnv* env, jobject jcaller) {
  auto* trace_plugin =
      new lynx::trace::FrameViewTracePluginAndroid(env, jcaller);
  return reinterpret_cast<jlong>(trace_plugin);
}

namespace lynx {
namespace trace {

void FrameViewTracePluginAndroid::DispatchBegin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_FrameViewTrace_startFrameViewTrace(env, weak_owner_.Get());
}

void FrameViewTracePluginAndroid::DispatchEnd() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_FrameViewTrace_stopFrameViewTrace(env, weak_owner_.Get());
}

std::string FrameViewTracePluginAndroid::Name() { return "FrameView"; }

}  // namespace trace
}  // namespace lynx
#endif
