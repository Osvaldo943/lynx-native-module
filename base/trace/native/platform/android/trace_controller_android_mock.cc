// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/trace/android/src/main/jni/gen/TraceController_jni.h"
#include "base/trace/android/src/main/jni/gen/TraceController_register_jni.h"
#include "base/trace/native/platform/android/trace_controller_android.h"

namespace lynx {
namespace jni {
bool RegisterJNIForTraceController(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static constexpr int kInvalidTraceSessionId = -1;

// static
jlong CreateTraceController(JNIEnv* env, jobject jcaller) {
  return reinterpret_cast<jlong>(lynx::trace::TraceController::Instance());
}

// static
jint StartTracing(JNIEnv* env, jobject jcaller, jlong ptr, jint buffer_size,
                  jobjectArray include_categories,
                  jobjectArray exclude_categories, jstring trace_file,
                  jboolean enableSystrace) {
  return kInvalidTraceSessionId;
}

// static
void StopTracing(JNIEnv* env, jobject jcaller, jlong ptr, jint session_id) {}

// static
void StartStartupTracingIfNeeded(JNIEnv* env, jobject jcaller, jlong ptr) {}

namespace lynx {
namespace trace {

std::string TraceControllerDelegateAndroid::GenerateTracingFileDir() {
  return "";
}

void TraceControllerDelegateAndroid::RefreshATraceTags() {}

void TraceControllerDelegateAndroid::SetIsTracingStarted(
    bool is_tracing_started) {}

}  // namespace trace
}  // namespace lynx
