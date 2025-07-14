// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_HARMONY_VALUE_IMPL_NAPI_H_
#define CORE_VALUE_WRAPPER_HARMONY_VALUE_IMPL_NAPI_H_

#include <node_api.h>

#include <cstddef>
#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace pub {

class ValueImplNapi : public Value {
 public:
  ValueImplNapi(napi_env env, napi_value value);
  ~ValueImplNapi() override;

  // Type
  int64_t Type() const override;
  bool IsUndefined() const override;
  bool IsBool() const override;
  bool IsInt32() const override;
  bool IsInt64() const override;
  bool IsUInt32() const override;
  bool IsUInt64() const override;
  bool IsDouble() const override;
  bool IsNumber() const override;

  bool IsNil() const override;
  bool IsString() const override;
  bool IsArray() const override;
  bool IsArrayBuffer() const override;
  bool IsMap() const override;
  bool IsFunction() const override;

  // Getter
  bool Bool() const override;
  double Double() const override;
  int32_t Int32() const override;
  uint32_t UInt32() const override;
  int64_t Int64() const override;
  uint64_t UInt64() const override;
  double Number() const override;
  uint8_t* ArrayBuffer() const override;
  const std::string& str() const override;
  int Length() const override;
  bool IsEqual(const Value& value) const override;

  // Iterator
  void ForeachArray(pub::ForeachArrayFunc func) const override;
  void ForeachMap(pub::ForeachMapFunc func) const override;

  // Find
  std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const override;
  bool Erase(uint32_t idx) const override;
  std::unique_ptr<Value> GetValueForKey(const std::string& key) const override;
  bool Erase(const std::string& key) const override;
  bool Contains(const std::string& key) const override;

  // Setter
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
  bool PushUInt32ToArray(uint32_t value) override;
  bool PushInt64ToArray(int64_t value) override;
  bool PushUInt64ToArray(uint64_t value) override;

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
  bool PushUInt32ToMap(const std::string& key, uint32_t value) override;
  bool PushInt64ToMap(const std::string& key, int64_t value) override;
  bool PushUInt64ToMap(const std::string& key, uint64_t value) override;

  napi_ref backend_value() const { return backend_value_; }

 private:
  bool PushNapiValueToArray(napi_value array, napi_value value);

  napi_env env_;
  napi_ref backend_value_;
  napi_valuetype type_;
  std::string cached_str_;
  uint32_t current_array_index_ = 0;
};

class PubValueFactoryNapi : public PubValueFactory {
 public:
  explicit PubValueFactoryNapi(napi_env env) : env_(env) {}
  ~PubValueFactoryNapi() override = default;
  std::unique_ptr<Value> CreateArray() override;
  std::unique_ptr<Value> CreateMap() override;
  std::unique_ptr<Value> CreateBool(bool value) override;
  std::unique_ptr<Value> CreateNumber(double value) override;
  std::unique_ptr<Value> CreateString(const std::string& value) override;
  std::unique_ptr<Value> CreateArrayBuffer(std::unique_ptr<uint8_t[]> value,
                                           size_t length) override;

 private:
  napi_env env_;
};

class ValueUtilsNapi {
 public:
  static napi_value ConvertPubValueToNapiValue(napi_env env,
                                               const Value& value);
  static napi_value ConvertPubValueToNapiArray(napi_env env,
                                               const Value& value);
  static napi_value ConvertPubValueToNapiObject(napi_env env,
                                                const Value& value);
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_HARMONY_VALUE_IMPL_NAPI_H_
