// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/common/android/lynx_inspector_owner_native_glue.h"

#include "core/base/android/jni_helper.h"
#include "core/services/recorder/native_module_recorder.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/config/devtool_config.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxInspectorOwner_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxInspectorOwner_register_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForLynxInspectorOwner(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx

void SetStopAtEntry(JNIEnv* env, jobject jcaller, jboolean stop,
                    jboolean isLepus) {
  lynx::devtool::DevToolConfig::SetStopAtEntry(stop, isLepus);
}

jboolean GetStopAtEntry(JNIEnv* env, jobject jcaller, jboolean isLepus) {
  return lynx::devtool::DevToolConfig::ShouldStopAtEntry(isLepus);
}

void InitRecorderConfig(JNIEnv* env, jobject jcaller, jstring filePath,
                        jint sessionID, jfloat screenWidth, jfloat screenHeight,
                        jlong recordID) {
  std::string path =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, filePath);
  lynx::tasm::recorder::RecorderController::InitConfig(
      path, sessionID, screenWidth, screenHeight, recordID);
}

void EndTestbench(JNIEnv* env, jobject jcaller, jstring filePath) {
  std::string path =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, filePath);
  lynx::tasm::replay::ReplayController::EndTest(path);
}

void RecordEventAndroid(JNIEnv* env, jobject jcaller, jint eventType,
                        jobjectArray values, jlong recordID) {
  lynx::base::android::ScopedGlobalJavaRef<jobjectArray> v =
      lynx::base::android::ScopedGlobalJavaRef<jobjectArray>(env, values);
  std::vector<std::string> args = lynx::base::android::JNIConvertHelper::
      ConvertJavaStringArrayToStringVector(env, v.Get());
  lynx::tasm::recorder::TestBenchBaseRecorder* ptr =
      static_cast<lynx::tasm::recorder::TestBenchBaseRecorder*>(
          lynx::tasm::recorder::RecorderController::
              GetTestBenchBaseRecorderInstance());
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  lynx::tasm::recorder::NativeModuleRecorder::GetInstance().RecordEventAndroid(
      args, eventType, recordID, ptr);
}

void SendFileByAgent(JNIEnv* env, jobject jcaller, jstring jni_type,
                     jstring jni_file) {
  std::string type =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, jni_type);
  std::string file =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, jni_file);
  lynx::tasm::replay::ReplayController::SendFileByAgent(type, file);
}

namespace lynx {
namespace devtool {

void LynxInspectorOwnerNativeGlue::Reload(JNIEnv* env, jobject obj,
                                          bool ignore_cache) {
  Java_LynxInspectorOwner_reload(env, obj, ignore_cache);
}

lynx::base::android::ScopedLocalJavaRef<jstring>
LynxInspectorOwnerNativeGlue::GetTemplateUrl(JNIEnv* env, jobject obj) {
  return Java_LynxInspectorOwner_getTemplateUrl(env, obj);
}

}  // namespace devtool
}  // namespace lynx
