// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_H_

#include <memory>
#include <string>
#include <vector>

#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer_builder.h"
#include "core/renderer/tasm/react/android/mapbuffer/map_buffer_builder.h"

namespace lynx {
namespace tasm {
class PropBundleCreatorAndroid : public PropBundleCreator {
 public:
  fml::RefPtr<PropBundle> CreatePropBundle() override;

  fml::RefPtr<PropBundle> CreatePropBundle(bool use_map_buffer) override;

  std::unique_ptr<PropArray> CreatePropArray() override;
};

class PropBundleAndroid : public PropBundle {
 public:
  PropBundleAndroid(const std::shared_ptr<base::android::JavaOnlyMap>& jni_map,
                    const std::shared_ptr<base::android::JavaOnlyArray>&
                        jni_event_handler_map);

  void SetNullProps(const char* key) override;
  void SetProps(const char* key, uint value) override;
  void SetProps(const char* key, int value) override;
  void SetProps(const char* key, const char* value) override;
  void SetProps(const char* key, bool value) override;
  void SetProps(const char* key, double value) override;
  void SetProps(const char* key, const pub::Value& value) override;
  void SetProps(const pub::Value& value) override;

  bool Contains(const char* key) const override;

  // styles.
  void SetNullPropsByID(CSSPropertyID id) override;
  void SetPropsByID(CSSPropertyID id, unsigned int value) override;
  void SetPropsByID(CSSPropertyID id, int value) override;
  void SetPropsByID(CSSPropertyID id, const char* value) override;
  void SetPropsByID(CSSPropertyID id, bool value) override;
  void SetPropsByID(CSSPropertyID id, double value) override;
  void SetPropsByID(CSSPropertyID id, const pub::Value& value) override;
  void SetPropsByID(CSSPropertyID id, const uint8_t* data,
                    size_t size) override;
  void SetPropsByID(CSSPropertyID id, const uint32_t* data,
                    size_t size) override;

  fml::RefPtr<PropBundle> ShallowCopy() override;

  void SetEventHandler(const pub::Value& event) override;
  void SetGestureDetector(const GestureDetector& detector) override;
  void ResetEventHandler() override;
  inline base::android::JavaOnlyMap* jni_map() const { return jni_map_.get(); }

  // Acquire Styled `ReadableMapBuffer` if `enable_map_buffer_` enabled.
  base::android::ScopedLocalJavaRef<jobject> GetStyleMapBuffer();

  inline base::android::JavaOnlyArray* jni_event_handler_map() const {
    return jni_event_handler_map_.get();
  }

  inline base::android::JavaOnlyArray* jni_gesture_detector_map() const {
    return jni_gesture_detector_map_.get();
  }

  static void AssembleArray(
      base::android::JavaOnlyArray* array, const pub::Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0, bool need_handle_null_value = true);

  static void AssembleMap(
      base::android::JavaOnlyMap* map, const char* key, const pub::Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0, bool need_handle_null_value = true);

  static void AssembleMapBuffer(lynx::base::android::MapBufferBuilder& builder,
                                uint16_t id, const pub::Value& value);

 private:
  PropBundleAndroid(bool use_map_buffer = false);
  friend class PropBundleCreatorAndroid;
  void CopyIfConst();
  void MarkConst() { is_const_ = true; }

 private:
  std::shared_ptr<base::android::JavaOnlyMap> jni_map_;
  std::shared_ptr<base::android::JavaOnlyArray> jni_event_handler_map_;
  std::unique_ptr<base::android::JavaOnlyArray> jni_gesture_detector_map_;

  base::android::MapBufferBuilder style_buffer_builder_{};
  std::unique_ptr<base::android::MapBuffer> style_map_buffer_{nullptr};

  bool is_const_{false};
  [[maybe_unused]] bool use_map_buffer_{false};

  PropBundleAndroid(const PropBundleAndroid&) = delete;
  PropBundleAndroid& operator=(const PropBundleAndroid&) = delete;
};

class PropArrayAndroid : public PropArray {
 public:
  PropArrayAndroid();

  void AddProp(int value) override;
  void AddProp(unsigned int value) override;
  void AddProp(const char* value) override;
  void AddProp(bool value) override;
  void AddProp(double value) override;

  std::optional<base::android::CompactArrayBuffer> GetArrayBuffer() {
    return ui_operation_batch_builder_->build();
  }

 private:
  std::optional<base::android::CompactArrayBufferBuilder>
      ui_operation_batch_builder_{std::nullopt};
  PropArrayAndroid(const PropArrayAndroid&) = delete;
  PropArrayAndroid& operator=(const PropArrayAndroid&) = delete;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_H_
