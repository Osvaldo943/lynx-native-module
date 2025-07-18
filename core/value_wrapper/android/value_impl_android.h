// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_ANDROID_VALUE_IMPL_ANDROID_H_
#define CORE_VALUE_WRAPPER_ANDROID_VALUE_IMPL_ANDROID_H_

#include <climits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_value.h"
#include "core/base/js_constants.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace pub {

class ValueImplAndroidFactory : public pub::PubValueFactory {
 public:
  std::unique_ptr<pub::Value> CreateBool(bool value) override;
  std::unique_ptr<pub::Value> CreateNumber(double value) override;
  std::unique_ptr<pub::Value> CreateString(const std::string& value) override;
  std::unique_ptr<pub::Value> CreateArrayBuffer(
      std::unique_ptr<uint8_t[]> value, size_t length) override;
  std::unique_ptr<pub::Value> CreateArray() override;
  std::unique_ptr<pub::Value> CreateMap() override;
  ~ValueImplAndroidFactory() override = default;
};

class ValueImplAndroid : public pub::Value {
 public:
  ValueImplAndroid(base::android::JavaValue&& value)
      : Value(pub::ValueBackendType::ValueBackendTypeJava),
        backend_value_(std::move(value)) {}

  ~ValueImplAndroid() override = default;

  // Type
  int64_t Type() const override {
    return static_cast<int64_t>(backend_value_.type());
  }
  bool IsUndefined() const override { return backend_value_.IsUndefined(); }

  bool IsBool() const override { return backend_value_.IsBool(); }
  bool IsInt32() const override { return backend_value_.IsInt32(); }
  bool IsInt64() const override { return backend_value_.IsInt64(); }
  bool IsUInt32() const override { return false; }
  bool IsUInt64() const override { return false; }
  bool IsDouble() const override { return backend_value_.IsDouble(); }
  bool IsNumber() const override { return backend_value_.IsNumber(); }

  bool IsNil() const override { return backend_value_.IsNull(); }
  bool IsString() const override { return backend_value_.IsString(); }
  bool IsArray() const override { return backend_value_.IsArray(); }
  bool IsArrayBuffer() const override { return backend_value_.IsArrayBuffer(); }
  bool IsMap() const override { return backend_value_.IsMap(); }
  bool IsFunction() const override { return false; }

  // Getter
  bool Bool() const override { return backend_value_.Bool(); }
  double Double() const override { return backend_value_.Double(); }
  int32_t Int32() const override { return backend_value_.Int32(); }
  uint32_t UInt32() const override { return 0; }
  int64_t Int64() const override { return backend_value_.Int64(); }
  uint64_t UInt64() const override { return 0; }
  double Number() const override { return backend_value_.Number(); }
  uint8_t* ArrayBuffer() const override;
  const std::string& str() const override { return backend_value_.String(); }
  int Length() const override { return backend_value_.Length(); }

  // Iterator
  void ForeachArray(pub::ForeachArrayFunc func) const override;
  void ForeachMap(pub::ForeachMapFunc func) const override;

  // Array
  bool PushValueToArray(const Value& value) override;
  bool PushValueToArray(std::unique_ptr<Value> value) override;
  bool PushNullToArray() override;
  bool PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                              size_t length) override;
  bool PushStringToArray(const std::string& value) override;
  bool PushBigIntToArray(const std::string& value) override;
  bool PushBoolToArray(bool value) override;
  bool PushDoubleToArray(double value) override;
  bool PushInt32ToArray(int32_t value) override;
  bool PushInt64ToArray(int64_t value) override;

  // Map
  bool PushValueToMap(const std::string& key, const Value& value) override;
  bool PushValueToMap(const std::string& key,
                      std::unique_ptr<Value> value) override;
  bool PushNullToMap(const std::string& key) override;
  bool PushArrayBufferToMap(const std::string& key,
                            std::unique_ptr<uint8_t[]> value,
                            size_t length) override;
  bool PushStringToMap(const std::string& key,
                       const std::string& value) override;
  bool PushBigIntToMap(const std::string& key,
                       const std::string& value) override;
  bool PushBoolToMap(const std::string& key, bool value) override;
  bool PushDoubleToMap(const std::string& key, double value) override;
  bool PushInt32ToMap(const std::string& key, int32_t value) override;
  bool PushInt64ToMap(const std::string& key, int64_t value) override;

  std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const override;
  bool Erase(uint32_t idx) const override { return false; }
  std::unique_ptr<Value> GetValueForKey(const std::string& key) const override;
  bool Erase(const std::string& key) const override { return false; }
  bool Contains(const std::string& key) const override;

  const base::android::JavaValue& backend_value() const {
    return backend_value_;
  }

  bool IsTransfer() const override { return backend_value_.IsTransfer(); }
  std::unique_ptr<pub::Value> ParseTransferValue(
      std::shared_ptr<PubValueFactory> value_factory) const override;

  bool IsLynxObject() const override { return backend_value_.IsLynxObject(); };

  std::unique_ptr<pub::Value> ParseLynxObject(
      std::shared_ptr<PubValueFactory> value_factory) const override;

 private:
  base::android::JavaValue backend_value_;
};

class ValueUtilsAndroid {
 public:
  static base::android::JavaValue ConvertValueToJavaValue(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
  static base::android::JavaValue ConvertValueToJavaArray(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
  static base::android::JavaValue ConvertValueToJavaMap(
      const Value& value,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_ANDROID_VALUE_IMPL_ANDROID_H_
