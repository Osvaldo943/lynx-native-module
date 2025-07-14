// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/android/lynx_template_bundle_android.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/runtime/jscache/android/bytecode_callback.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#include "core/shell/android/tasm_platform_invoker_android.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "platform/android/lynx_android/src/main/jni/gen/TemplateBundle_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/TemplateBundle_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForTemplateBundle(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace {
/**
 * Convert native PageConfig to java map
 */
std::optional<lynx::base::android::JavaOnlyMap> GetPageConfigMap(
    JNIEnv* env, lynx::tasm::LynxTemplateBundle* bundle) {
  if (!bundle->IsCard()) {
    return std::nullopt;
  }
  const std::shared_ptr<lynx::tasm::PageConfig>& page_config =
      bundle->GetPageConfig();
  if (!page_config) {
    return std::nullopt;
  }
  page_config->MarkPostToPlatform();
  return lynx::shell::TasmPlatformInvokerAndroid::ConvertToJavaOnlyMap(
      page_config);
}
}  // namespace

jlong ParseTemplate(JNIEnv* env, jclass jcaller, jbyteArray j_binary,
                    jobjectArray j_buffer) {
  auto binary =
      lynx::base::android::JNIConvertHelper::ConvertJavaBinary(env, j_binary);
  auto reader =
      lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(std::move(binary));
  if (reader.Decode()) {
    // decode success.
    lynx::tasm::LynxTemplateBundle* bundle =
        new lynx::tasm::LynxTemplateBundle(reader.GetTemplateBundle());
    bundle->PrepareVMByConfigs();
    auto page_config = GetPageConfigMap(env, bundle);
    env->SetObjectArrayElement(
        j_buffer, 1, page_config ? page_config->jni_object() : nullptr);
    return reinterpret_cast<int64_t>(bundle);
  } else {
    // decode failed.
    LOGE("ParseTemplate failed. error_msg is : " << reader.error_message_);
    auto j_err_str =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
            env, reader.error_message_);
    env->SetObjectArrayElement(j_buffer, 0, j_err_str.Get());
    return 0;
  }
}

jobject GetExtraInfo(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (bundle) {
    lynx::lepus::Value extra_info = bundle->GetExtraInfo();
    lynx::tasm::LepusEncoder encoder;
    std::vector<int8_t> encoded_data = encoder.EncodeMessage(extra_info);
    auto buffer =
        env->NewDirectByteBuffer(encoded_data.data(), encoded_data.size());
    auto result_from_java = Java_TemplateBundle_decodeByteBuffer(env, buffer);
    return env->NewLocalRef(result_from_java.Get());  // NOLINT
  }
  return nullptr;
}

jboolean GetContainsElementTree(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (bundle) {
    return bundle->GetContainsElementTree();
  }
  return false;
}

void ReleaseBundle(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  delete bundle;
}

void PostJsCacheGenerationTask(JNIEnv* env, jclass jcaller, jlong bundle,
                               jstring bytecodeSourceUrl, jboolean useV8,
                               jobject callback) {
  std::string template_url =
      lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                             bytecodeSourceUrl);
  lynx::tasm::LynxTemplateBundle* template_bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(bundle);
  std::unique_ptr<lynx::piper::cache::BytecodeGenerateCallback>
      bytecode_callback = nullptr;
  if (nullptr != callback) {
    lynx::base::android::ScopedGlobalJavaRef<jobject> jni_object(env, callback);
    bytecode_callback = std::make_unique<
        lynx::piper::cache::BytecodeGenerateCallback>(
        [jni_object = std::move(jni_object)](
            std::string error_msg,
            std::unordered_map<std::string,
                               std::shared_ptr<lynx::piper::Buffer>>
                buffers) {
          JNIEnv* env = lynx::base::android::AttachCurrentThread();
          lynx::base::android::ScopedLocalJavaRef<jstring> jni_error_msg;
          if (!error_msg.empty()) {
            jni_error_msg =
                lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
                    env, error_msg);
          }
          auto java_map = lynx::base::android::JavaOnlyMap();
          for (const auto& iter : buffers) {
            if (nullptr != iter.second) {
              jobject byte_buffer = env->NewDirectByteBuffer(
                  const_cast<void*>(
                      static_cast<const void*>(iter.second->data())),
                  iter.second->size());
              java_map.PushByteBuffer(iter.first, byte_buffer);
            }
          }
          lynx::piper::cache::OnBytecodeResponse(
              env, std::move(jni_object), std::move(jni_error_msg), java_map);
        });
  }
  // base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
  lynx::piper::cache::JsCacheManagerFacade::PostCacheGenerationTask(
      *template_bundle, template_url,
      useV8 ? lynx::piper::JSRuntimeType::v8
            : lynx::piper::JSRuntimeType::quickjs,
      std::move(bytecode_callback));
}

jboolean ConstructContext(JNIEnv* env, jclass jcaller, jlong ptr, jint count) {
  lynx::tasm::LynxTemplateBundle* bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  return bundle && bundle->PrepareLepusContext(count);
}

void InitWithOption(JNIEnv* env, jclass jcaller, jlong ptr,
                    jint context_pool_size,
                    jboolean enable_context_auto_generate) {
  lynx::tasm::LynxTemplateBundle* bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (!bundle) {
    return;
  }
  bundle->PrepareLepusContext(context_pool_size);
  bundle->SetEnableVMAutoGenerate(enable_context_auto_generate);
}

namespace lynx {
namespace tasm {
lynx::base::android::ScopedLocalJavaRef<jobject>
ConstructJTemplateBundleFromNative(LynxTemplateBundle bundle) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto* native_bundle_ptr = new tasm::LynxTemplateBundle(std::move(bundle));
  auto page_config = GetPageConfigMap(env, native_bundle_ptr);
  return Java_TemplateBundle_fromNative(
      env, reinterpret_cast<int64_t>(native_bundle_ptr),
      page_config ? page_config->jni_object() : nullptr);
}
}  // namespace tasm
}  // namespace lynx
