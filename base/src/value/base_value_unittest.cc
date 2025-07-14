// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/base_value.h"

#include <functional>

#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class BaseValueTest : public ::testing::Test {
 protected:
  BaseValueTest() = default;
  ~BaseValueTest() override = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(BaseValueTest, BaseValueNull) {
  lepus::Value v1;
  ASSERT_TRUE(v1.IsNil());
  ASSERT_TRUE(!v1.IsReference());
  ASSERT_TRUE(v1.IsFalse());
  ASSERT_TRUE(v1.Type() == lepus::Value_Nil);
  lepus::Value v2;
  v2.SetNil();
  ASSERT_TRUE(v2.IsNil());
  ASSERT_TRUE(v2.IsEmpty());

  lepus::Value v3;
  v3.SetUndefined();
  ASSERT_TRUE(v3.IsUndefined());
  ASSERT_TRUE(v3.IsEmpty());
  ASSERT_TRUE(v3.IsFalse());
  ASSERT_TRUE(v3.Type() == lepus::Value_Undefined);
}

TEST_F(BaseValueTest, BaseValueNumber) {
  {
    lepus::Value v1(true, true);
    ASSERT_TRUE(v1.IsNaN());
    ASSERT_TRUE(v1.NaN());
    ASSERT_TRUE(v1.Type() == lepus::Value_NaN);
    lepus::Value v2;
    v2.SetNan(true);
    ASSERT_TRUE(v2.IsNaN());
    ASSERT_TRUE(v2.NaN());
    ASSERT_TRUE(v2.IsFalse());
  }
  {
    lepus::Value v3((int32_t)10);
    ASSERT_TRUE(v3.IsInt32());
    ASSERT_TRUE(v3.IsNumber());
    ASSERT_TRUE(v3.Int32() == 10);
    ASSERT_TRUE(v3.Type() == lepus::Value_Int32);
    v3.SetNumber((uint32_t)100);
    ASSERT_TRUE(v3.IsUInt32());
    ASSERT_TRUE(v3.UInt32() == 100);
    ASSERT_TRUE(v3.Type() == lepus::Value_UInt32);
    lepus::Value v4((uint32_t)50);
    ASSERT_TRUE(v4.IsUInt32());
    ASSERT_TRUE(v4.IsNumber());
    ASSERT_TRUE(v4.UInt32() == 50);
    v4.SetNumber((int32_t)101);
    ASSERT_TRUE(v4.IsInt32());
    ASSERT_TRUE(v4.Int32() == 101);
    int64_t num1 = (int64_t)INT32_MAX + 1;
    lepus::Value v5(num1);
    ASSERT_TRUE(v5.IsInt64());
    ASSERT_TRUE(v5.IsNumber());
    ASSERT_TRUE(v5.Int64() == num1);
    ASSERT_TRUE(v5.Type() == lepus::Value_Int64);
    v5.SetNumber((int32_t)101);
    ASSERT_TRUE(v5.IsInt32());
    ASSERT_TRUE(v5.Int32() == 101);
    uint64_t num2 = (uint64_t)INT32_MAX + 10;
    lepus::Value v6(num2);
    ASSERT_TRUE(v6.IsUInt64());
    ASSERT_TRUE(v6.IsNumber());
    ASSERT_TRUE(v6.UInt64() == num2);
    ASSERT_TRUE(v6.Type() == lepus::Value_UInt64);
    v6.SetNumber((int32_t)101);
    ASSERT_TRUE(v6.IsInt32());
    ASSERT_TRUE(v6.Int32() == 101);
    lepus::Value v7(3.14f);
    ASSERT_TRUE(v7.IsDouble());
    ASSERT_TRUE(v7.IsNumber());
    ASSERT_TRUE(v7.Double() == 3.14f);
    ASSERT_TRUE(v7.Type() == lepus::Value_Double);
    v7.SetNumber((int32_t)101);
    ASSERT_TRUE(v7.IsInt32());
    ASSERT_TRUE(v7.Int32() == 101);
    lepus::Value v8((uint8_t)3);
    ASSERT_TRUE(v8.IsUInt32());
    ASSERT_TRUE(v8.UInt32() == 3);
  }
  {
    lepus::Value v14((int32_t)10);
    lepus::Value v15((int32_t)5);
    ASSERT_TRUE((v14 / v15).Number() == 2);
    ASSERT_TRUE((v14 * v15).Number() == 50);
    ASSERT_TRUE((v14 + v15).Number() == 15);
    ASSERT_TRUE((v14 - v15).Number() == 5);
    ASSERT_TRUE((v14 % v15).Number() == 0);
  }
}

TEST_F(BaseValueTest, BaseValueString) {
  {
    base::String s1("test");
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsString());
    ASSERT_TRUE(v1.StdString() == "test");
    ASSERT_TRUE(v1.GetLength() == 4);
    ASSERT_TRUE(v1.IsReference());
    ASSERT_TRUE(v1.Type() == lepus::Value_String);
    lepus::Value b1(true);
    ASSERT_TRUE(b1.IsBool());
    ASSERT_TRUE(b1.Type() == lepus::Value_Bool);
    ASSERT_TRUE(b1.StdString() == "true");
    base::String s2("test2");
    lepus::Value v2(std::move(s2));
    ASSERT_TRUE(v2.IsString());
    ASSERT_TRUE(v2.StdString() == "test2");
  }
  {
    lepus::Value v3("abcd");
    ASSERT_TRUE(v3.IsString());
    ASSERT_TRUE(v3.StdString() == "abcd");
    lepus::Value v4(v3);
    ASSERT_TRUE(v4.IsString());
    ASSERT_TRUE(v4.StdString() == "abcd");
    auto str = v4.String();
    ASSERT_TRUE(str.str() == "abcd");
    auto str1 = lepus::Value(v4).String();
    ASSERT_TRUE(str1.str() == "abcd");
  }
  {
    lepus::Value v5;
    base::String s3("test");
    v5.SetString(s3);
    ASSERT_TRUE(v5.IsString());
    ASSERT_TRUE(v5.StdString() == "test");
    ASSERT_TRUE(v5.ToString() == "test");
    lepus::Value v6;
    v6.SetString(base::String("aaa"));
    ASSERT_TRUE(v6.IsString());
    ASSERT_TRUE(v6.StdString() == "aaa");
    auto r2 = v5.GetProperty(2);
    ASSERT_TRUE(r2.StdString() == "s");
    lepus::Value v7("");
    ASSERT_TRUE(v7.IsFalse());
  }
}

TEST_F(BaseValueTest, BaseValueArray) {
  auto arr1 = lepus::CArray::Create();
  arr1->push_back(lepus::Value((int32_t)101));
  arr1->push_back(lepus::Value((int64_t)2001));
  arr1->push_back(lepus::Value(5.645f));
  arr1->push_back(lepus::Value(false));
  arr1->push_back(lepus::Value("testing"));
  lepus::Value v1(arr1);
  ASSERT_TRUE(v1.Type() == lepus::Value_Array);
  ASSERT_TRUE(v1.IsArray());
  auto r0 = v1.GetProperty(0);
  ASSERT_TRUE(r0.Int32() == 101);
  auto r1 = v1.GetProperty(1);
  ASSERT_TRUE(r1.Int64() == 2001);
  auto r2 = v1.GetProperty(2);
  ASSERT_TRUE(r2.Double() == 5.645f);
  auto r3 = v1.GetProperty(3);
  ASSERT_TRUE(r3.Bool() == false);
  auto r4 = v1.GetProperty(4);
  ASSERT_TRUE(r4.StdString() == "testing");
  v1.SetProperty(5, lepus::Value(true));
  auto r5 = v1.GetProperty(5);
  ASSERT_TRUE(r5.Bool() == true);
  lepus::Value id6("666");
  v1.SetProperty(6, id6);
  auto r6 = v1.GetProperty(6);
  ASSERT_TRUE(r6.StdString() == "666");
  ASSERT_TRUE(v1.GetLength() == 7);
  ASSERT_TRUE(v1.Array()->size() == 7);
}

TEST_F(BaseValueTest, BaseValueMap) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", lepus::Value((int32_t)101));
  dict->SetValue("key2", lepus::Value((int64_t)2001));
  dict->SetValue("key3", lepus::Value(5.645f));
  dict->SetValue("key4", lepus::Value(false));
  dict->SetValue("key5", lepus::Value("testing"));
  lepus::Value v1(dict);
  ASSERT_TRUE(v1.Type() == lepus::Value_Table);
  ASSERT_TRUE(v1.IsTable());
  auto r0 = v1.GetProperty("key1");
  ASSERT_TRUE(r0.Int32() == 101);
  auto r1 = v1.GetProperty("key2");
  ASSERT_TRUE(r1.Int64() == 2001);
  auto r2 = v1.GetProperty("key3");
  ASSERT_TRUE(r2.Double() == 5.645f);
  auto r3 = v1.GetProperty("key4");
  ASSERT_TRUE(r3.Bool() == false);
  auto r4 = v1.GetProperty("key5");
  ASSERT_TRUE(r4.StdString() == "testing");
  v1.SetProperty("key6", lepus::Value(true));
  auto r5 = v1.GetProperty("key6");
  ASSERT_TRUE(r5.Bool() == true);
  lepus::Value id6("666");
  v1.SetProperty("key7", id6);
  auto r6 = v1.GetProperty("key7");
  ASSERT_TRUE(r6.StdString() == "666");
  lepus::Value id7("abc");
  base::String key8("key8");
  v1.SetProperty(key8, id7);
  auto r7 = v1.GetProperty("key8");
  ASSERT_TRUE(r7.StdString() == "abc");
  ASSERT_TRUE(v1.GetLength() == 8);
  ASSERT_TRUE(v1.Table()->size() == 8);
  ASSERT_TRUE(v1.Contains("key1"));

  for (auto& it : *(dict.get())) {
    auto key = it.first.str();
    if (key == "key8") {
      ASSERT_TRUE(it.second.StdString() == "abc");
    }
  }
}

TEST_F(BaseValueTest, BaseValueArrayBuffer) {
  auto buffer1 = lepus::ByteArray::Create();
  lepus::Value v1(buffer1);
  ASSERT_TRUE(v1.IsByteArray());
  auto buffer2 = lepus::ByteArray::Create();
  lepus::Value v2(std::move(buffer2));
  ASSERT_TRUE(v2.IsByteArray());
  ASSERT_TRUE(v2.ByteArray());
  ASSERT_TRUE(v2.Type() == lepus::Value_ByteArray);
}

TEST_F(BaseValueTest, BaseValuePointer) {
  int* a = new int;
  *a = 10;
  lepus::Value v1(a);
  ASSERT_TRUE(v1.IsCPointer());
  ASSERT_TRUE(v1.Type() == lepus::Value_CPointer);
  int* b = reinterpret_cast<int*>(v1.CPoint());
  ASSERT_TRUE(*a == *b);
  lepus::Value v2;
  int* c = new int;
  v2.SetCPoint(c);
  int* d = reinterpret_cast<int*>(v2.CPoint());
  ASSERT_TRUE(v2.IsCPointer());
  ASSERT_TRUE(!v2.IsReference());
  ASSERT_TRUE(*c == *d);
}

TEST_F(BaseValueTest, BaseValueCloneValue) {
  auto arr_ref = lepus::CArray::Create();
  auto dict_ref = lepus::Dictionary::Create();
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", lepus::Value("test_val"));
  dict->SetValue("key2", lepus::Value());
  lepus::Value undefined;
  undefined.SetUndefined();
  dict->SetValue("key3", undefined);
  dict->SetValue("key4", lepus::Value((int32_t)10));
  dict->SetValue("key5", lepus::Value((uint32_t)11));
  dict->SetValue("key6", lepus::Value((int64_t)12));
  dict->SetValue("key7", lepus::Value((uint64_t)13));
  dict->SetValue("key8", lepus::Value(3.45f));
  dict->SetValue("key9", lepus::Value("string"));
  auto arr1 = lepus::CArray::Create();
  arr1->push_back(lepus::Value(false));
  arr1->push_back(lepus::Value("str"));
  dict->SetValue("key10", lepus::Value(arr1));
  auto dict1 = lepus::Dictionary::Create();
  dict1->SetValue("key1", lepus::Value("string"));
  dict->SetValue("key11", lepus::Value(dict1));
  lepus::Value v1(dict);
  auto ret1 = lepus::Value::Clone(v1, true);
  ASSERT_TRUE(ret1.IsTable());
  auto r1 = ret1.GetProperty("key1");
  ASSERT_TRUE(r1.StdString() == "test_val");
  ASSERT_TRUE(!r1.IsJSString());
  auto r2 = ret1.GetProperty("key2");
  ASSERT_TRUE(r2.IsNil());
  auto r3 = ret1.GetProperty("key3");
  ASSERT_TRUE(r3.IsUndefined());
  auto r4 = ret1.GetProperty("key4");
  ASSERT_TRUE(r4.Int32() == 10);
  auto r5 = ret1.GetProperty("key5");
  ASSERT_TRUE(r5.UInt32() == 11);
  auto r6 = ret1.GetProperty("key6");
  ASSERT_TRUE(r6.Int64() == 12);
  auto r7 = ret1.GetProperty("key7");
  ASSERT_TRUE(r7.UInt64() == 13);
  auto r8 = ret1.GetProperty("key8");
  ASSERT_TRUE(r8.Double() == 3.45f);
  auto r9 = ret1.GetProperty("key9");
  ASSERT_TRUE(r9.StdString() == "string");
  auto r10 = ret1.GetProperty("key10");
  ASSERT_TRUE(r10.Array()->size() == 2);
  ASSERT_TRUE(r10.Array()->get(1).StdString() == "str");
  auto r11 = ret1.GetProperty("key11");
  ASSERT_TRUE(r11.Table()->size() == 1);
  ASSERT_TRUE(ret1.IsEqual(v1));
  auto ret2 = lepus::Value::Clone(v1, false);
  ASSERT_TRUE(ret2.GetLength() == 11);
  ASSERT_TRUE(ret2.IsEqual(v1));
  ret2.SetProperty("key12", lepus::Value("value12"));
  ASSERT_TRUE(ret2.GetLength() == 12);
  ASSERT_TRUE(v1.GetLength() == 11);
  ASSERT_TRUE(!ret2.GetProperty("key1").IsJSString());
  ASSERT_TRUE(!ret2.IsEqual(v1));
  auto ret3 = lepus::Value::ShallowCopy(v1, true);
  ASSERT_TRUE(ret3.IsEqual(v1));
  auto ret4 = lepus::Value::ShallowCopy(v1, false);
  ASSERT_TRUE(ret4.IsEqual(v1));
}

}  // namespace base
}  // namespace lynx
