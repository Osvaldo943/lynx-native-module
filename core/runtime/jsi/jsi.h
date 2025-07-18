/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSI_H_
#define CORE_RUNTIME_JSI_JSI_H_

#include <cassert>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/expected.h"
#include "base/include/log/logging.h"
#include "base/include/vector.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/inspector/console_message_postman.h"
#include "core/inspector/observer/inspector_runtime_observer_ng.h"

#define BUILD_JSI_NATIVE_EXCEPTION(message) \
  piper::JSINativeException(message, __FUNCTION__, __FILE__, __LINE__)
#define ADD_STACK(e) e.AddStack(__FUNCTION__, __FILE__, __LINE__)
#define REPORT_JSI_NATIVE_EXCEPTION(message)                  \
  piper::JSINativeExceptionCollector::Instance()->AddStack(   \
      __FUNCTION__, __FILE__, __LINE__);                      \
  piper::JSINativeExceptionCollector::Instance()->SetMessage( \
      std::move(message));

namespace lynx {
namespace piper {
constexpr char kErrorInfoSeparator[] = " ";

class BASE_EXPORT Buffer {
 public:
  BASE_EXPORT virtual ~Buffer() = default;
  virtual size_t size() const = 0;
  virtual const uint8_t* data() const = 0;
};

class StringBuffer : public Buffer {
 public:
  StringBuffer(std::string s) : s_(std::move(s)) {}
  size_t size() const override { return s_.size(); }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(s_.data());
  }

 private:
  std::string s_;
};

class ByteBuffer : public Buffer {
 public:
  explicit ByteBuffer(std::vector<uint8_t>&& data) : data_(std::move(data)) {}
  size_t size() const override { return data_.size(); }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(data_.data());
  }

 private:
  std::vector<uint8_t> data_;
};

using BytecodeGetter =
    base::MoveOnlyClosure<std::shared_ptr<piper::Buffer>, const std::string&>;

/// PreparedJavaScript is a base class repesenting JavaScript which is in a form
/// optimized for execution, in a runtime-specific way. Construct one via
/// piper::Runtime::prepareJavaScript().
/// ** This is an experimental API that is subject to change. **
class PreparedJavaScript {
 protected:
  PreparedJavaScript() = default;

 public:
  BASE_EXPORT virtual ~PreparedJavaScript() = 0;
};

class Runtime;
class Pointer;
class PropNameID;
class Symbol;
class String;
class Object;
class BigInt;
class Array;
class ArrayBuffer;
class Function;
class Value;
class Instrumentation;
class Scope;
class JSIException;
class JSINativeException;
class JSError;
class Global;
class JSIExceptionHandler;
class VMInstance;
class JSIContext;
class JSIObserver;
class StartupData;
/// A function which has this type can be registered as a function
/// callable from JavaScript using Function::createFromHostFunction().
/// When the function is called, args will point to the arguments, and
/// count will indicate how many arguments are passed.  The function
/// can return a Value to the caller, or throw an exception.  If a C++
/// exception is thrown, a JS Error will be created and thrown into
/// JS; if the C++ exception extends std::exception, the Error's
/// message will be whatever what() returns. Note that it is undefined
/// whether HostFunctions may or may not be called in strict mode; that is
/// `thisVal` can be any value - it will not necessarily be coerced to an
/// object or or set to the global object.
using HostFunctionType =
    base::MoveOnlyClosure<base::expected<Value, JSINativeException>, Runtime&,
                          const Value&, const Value*, size_t>;

/// An object which implements this interface can be registered as an
/// Object with the JS runtime.
class HostObject {
 public:
  // The C++ object's dtor will be called when the GC finalizes this
  // object.  (This may be as late as when the Runtime is shut down.)
  // You have no control over which thread it is called on.  This will
  // be called from inside the GC, so it is unsafe to do any VM
  // operations which require a Runtime&.  Derived classes' dtors
  // should also avoid doing anything expensive.  Calling the dtor on
  // a jsi object is explicitly ok.  If you want to do JS operations,
  // or any nontrivial work, you should add it to a work queue, and
  // manage it externally.
  virtual ~HostObject() = default;

  // When JS wants a property with a given name from the HostObject,
  // it will call this method.  If it throws an exception, the call
  // will throw a JS \c Error object. By default this returns undefined.
  // \return the value for the property.
  virtual Value get(Runtime*, const PropNameID& name);

  // When JS wants to set a property with a given name on the HostObject,
  // it will call this method. If it throws an exception, the call will
  // throw a JS \c Error object. By default this throws a type error exception
  // mimicking the behavior of a frozen object in strict mode.
  virtual void set(Runtime*, const PropNameID& name, const Value& value);

  // When JS wants a list of property names for the HostObject, it will
  // call this method. If it throws an exception, the call will thow a
  // JS \c Error object. The default implementation returns empty vector.
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt);
};

template <typename Rt, typename Host>
class HostObjectWrapperBase {
 public:
  explicit HostObjectWrapperBase(Rt* rt, std::shared_ptr<Host> host_object)
      : rt_(rt),
        host_obj_(host_object),
        is_runtime_destroyed_(rt->GetRuntimeDestroyedFlag()) {
    rt->AddHostObject(std::move(host_object));
  }

  virtual ~HostObjectWrapperBase() {
    auto lock_host = host_obj_.lock();
    if (!(*is_runtime_destroyed_) && lock_host) {
      rt_->RemoveHostObject(lock_host.get());
    }
    rt_ = nullptr;
  }

  Rt* GetRuntime() {
    if (!(*is_runtime_destroyed_) && rt_ != nullptr) {
      return rt_;
    }
    return nullptr;
  }

  std::shared_ptr<Host> GetHost() {
    return *is_runtime_destroyed_ ? nullptr : host_obj_.lock();
  }

  bool GetRuntimeAndHost(Rt*& rt, std::shared_ptr<Host>& host) {
    auto lock_obj = host_obj_.lock();
    if (!(*is_runtime_destroyed_) && lock_obj && rt_ != nullptr) {
      rt = rt_;
      host = std::move(lock_obj);
      return true;
    }
    return false;
  }

 private:
  Rt* rt_;
  std::weak_ptr<Host> host_obj_;
  std::shared_ptr<bool> is_runtime_destroyed_;
};

enum class JSRuntimeCreatedType {
  unknown = 0,
  vm_context,           // create vm + context
  context,              // create only context
  none_vm_none_context  // create noe vm, none context
};

enum class JSRuntimeType { v8 = 0, jsc, quickjs, jsvm };

/// Represents a JS runtime.  Movable, but not copyable.  Note that
/// this object may not be thread-aware, but cannot be used safely from
/// multiple threads at once.  The application is responsible for
/// ensuring that it is used safely.  This could mean using the
/// Runtime from a single thread, using a mutex, doing all work on a
/// serial queue, etc.  This restriction applies to the methods of
/// this class, and any method in the API which take a Runtime& as an
/// argument.  Destructors (all but ~Scope), operators, or other methods
/// which do not take Runtime& as an argument are safe to call from any
/// thread, but it is still forbidden to make write operations on a single
/// instance of any class from more than one thread.  In addition, to
/// make shutdown safe, destruction of objects associated with the Runtime
/// must be destroyed before the Runtime is destroyed, or from the
/// destructor of a managed HostObject or HostFunction.  Informally, this
/// means that the main source of unsafe behavior is to hold a jsi object
/// in a non-Runtime-managed object, and not clean it up before the Runtime
/// is shut down.  If your lifecycle is such that avoiding this is hard,
/// you will probably need to do use your own locks.
class BASE_EXPORT Runtime {
 public:
  virtual void InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                           std::shared_ptr<JSIExceptionHandler> handler) = 0;
  virtual JSRuntimeType type() = 0;
  virtual void SetGCPauseSuppressionMode(bool mode){};
  virtual bool GetGCPauseSuppressionMode() { return false; };
  virtual void SetObserver(JSIObserver* observer){};
  int64_t getRuntimeId() const { return runtime_id_; }
  bool getGCFlag() { return gc_flag_; }
  void setRuntimeId(int64_t rt_id) { runtime_id_ = rt_id; }
  const std::string& getGroupId() { return group_id_; }
  BASE_EXPORT_FOR_DEVTOOL void setGroupId(const std::string& group_id) {
    group_id_ = group_id;
  }
  // will override in quickjsruntime, this version just works as a sentinel
  virtual bool setPropertyValueGC(Object& object, const char* name,
                                  const piper::Value& value) {
    abort();
    return true;
  }

  std::shared_ptr<bool> GetRuntimeDestroyedFlag() {
    return is_runtime_destroyed_;
  }

  void SetInJSErrorConstructionProcessing(bool flag) {
    is_in_js_error_construction_processing = flag;
  }
  bool IsInJSErrorConstructionProcessing() {
    return is_in_js_error_construction_processing;
  }

  void SetCircularDataCheckFlag(bool enable_check) {
    circular_data_check_flag_ =
        enable_check ? CircularCheckFlags::ENABLE : CircularCheckFlags::DISABLE;
  }
  bool IsEnableCircularDataCheck() {
    return circular_data_check_flag_ == CircularCheckFlags::ENABLE;
  }
  bool IsCircularDataCheckUnset() {
    return circular_data_check_flag_ == CircularCheckFlags::UNSET;
  }

  void SetEnableJsBindingApiThrowException(bool enable) {
    enable_js_binding_api_throw_exception_ = enable;
  }

  bool IsEnableJsBindingApiThrowException() const {
    return enable_js_binding_api_throw_exception_;
  }

  void reportJSIException(const JSIException& exception);
  virtual std::shared_ptr<VMInstance> createVM(
      const StartupData* data) const = 0;
  virtual std::shared_ptr<VMInstance> getSharedVM() = 0;
  virtual std::shared_ptr<JSIContext> createContext(
      std::shared_ptr<VMInstance> vm) const = 0;
  virtual std::shared_ptr<JSIContext> getSharedContext() = 0;
  // virtual StartupData createDefaultStartupData() = 0;
  // Should be override by its decendant to indicate if still valid for
  // shard JS context wrapper case
  virtual bool Valid() const { return true; }

  BASE_EXPORT virtual ~Runtime() = default;
  void setCreatedType(JSRuntimeCreatedType type) { created_type_ = type; }
  JSRuntimeCreatedType getCreatedType() { return created_type_; }

  /// Evaluates the given JavaScript \c source.  \c sourceURL is used
  /// to annotate the stack trace if there is an exception.  The
  /// contents may be utf8-encoded JS source code, or binary bytecode
  /// whose format is specific to the implementation.  If the input
  /// format is unknown, or evaluation causes an error, a JSIException
  /// will be thrown.
  /// Note this function should ONLY be used when there isn't another means
  /// through the JSI API. For example, it will be much slower to use this to
  /// call a global function than using the JSI APIs to read the function
  /// property from the global object and then calling it explicitly.
  virtual base::expected<Value, JSINativeException> evaluateJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& sourceURL) = 0;

  virtual base::expected<Value, JSINativeException> evaluateJavaScriptBytecode(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& source_url) = 0;

  /// Prepares to evaluate the given JavaScript \c buffer by processing it into
  /// a form optimized for execution. This may include pre-parsing, compiling,
  /// etc. If the input is invalid (for example, cannot be parsed), a
  /// JSIException will be thrown. The resulting object is tied to the
  /// particular concrete type of Runtime from which it was created. It may be
  /// used (via evaluatePreparedJavaScript) in any Runtime of the same concrete
  /// type.
  /// The PreparedJavaScript object may be passed to multiple VM instances, so
  /// they can all share and benefit from the prepared script.
  /// As with evaluateJavaScript(), using JavaScript code should be avoided
  /// when the JSI API is sufficient.
  virtual std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer>& buffer, std::string source_url) = 0;

  /// Evaluates a PreparedJavaScript. If evaluation causes an error, a
  /// JSIException will be thrown.
  /// As with evaluateJavaScript(), using JavaScript code should be avoided
  /// when the JSI API is sufficient.
  virtual base::expected<Value, JSINativeException> evaluatePreparedJavaScript(
      const std::shared_ptr<const PreparedJavaScript>& js) = 0;

  /// \return the global object
  virtual Object global() = 0;

  /// \return a short printable description of the instance.  This
  /// should only be used by logging, debugging, and other
  /// developer-facing callers.
  virtual std::string description() = 0;

  /// \return whether or not the underlying runtime supports debugging via the
  /// Chrome remote debugging protocol.
  ///
  /// NOTE: the API for determining whether a runtime is debuggable and
  /// registering a runtime with the debugger is still in flux, so please don't
  /// use this API unless you know what you're doing.
  virtual bool isInspectable() = 0;

  /// \return an interface to extract metrics from this \c Runtime.  The default
  /// implementation of this function returns an \c Instrumentation instance
  /// which returns no metrics.
  virtual Instrumentation& instrumentation();

  void SetEnableUserBytecode(bool enable) { enable_user_bytecode_ = enable; }

  void SetBytecodeSourceUrl(const std::string& url) {
    bytecode_source_url_ = url;
  }

  void SetBytecodeGetter(BytecodeGetter getter) {
    if (getter) {
      bytecode_getter_ = std::make_unique<BytecodeGetter>(std::move(getter));
    } else {
      bytecode_getter_.reset();
    }
  }

  // Post a GC request.
  // In QuickJS and V8 engine, `RequestGC` will trigger a synchronized full
  // GC, which will block the current thread until the GC is finished. In
  // JavascriptCore, `RequestGC` will trigger a GC notification, and the time to
  // do GC is decided by the JS engine.
  virtual void RequestGC() = 0;

  // In QuickJS and V8 engine, `RequestGCForTesting` is no difference with
  // `RequestGC`. But in JavascriptCore, it will trigger a synchronized GC. Only
  // use for Testing.
  virtual void RequestGCForTesting() { RequestGC(); };

  virtual void InitInspector(
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) {}
  virtual void DestroyInspector() {}

  template <typename Host>
  void AddHostObject(std::shared_ptr<Host> host_object) {
    static_assert(std::is_same<Host, HostObject>::value ||
                      std::is_same<Host, HostFunctionType>::value,
                  "Host must be HostObject or HostFunctionType.");
    if constexpr (std::is_same<Host, HostObject>::value) {
      host_object_containers_.emplace(host_object.get(),
                                      std::move(host_object));
    } else {
      host_function_containers_.emplace(host_object.get(),
                                        std::move(host_object));
    }
  }

  template <typename Host>
  void RemoveHostObject(Host* host_object) {
    static_assert(std::is_same<Host, HostObject>::value ||
                      std::is_same<Host, HostFunctionType>::value,
                  "Host must be HostObject or HostFunctionType.");
    if constexpr (std::is_same<Host, HostObject>::value) {
      host_object_containers_.erase(host_object);
    } else {
      host_function_containers_.erase(host_object);
    }
  }

  // Potential optimization: avoid the cloneFoo() virtual dispatch,
  // and instead just fix the number of fields, and copy them, since
  // in practice they are trivially copyable.  Sufficient use of
  // rvalue arguments/methods would also reduce the number of clones.

  struct PointerValue {
    PointerValue() {}
    virtual void invalidate(){

    };

   protected:
    virtual ~PointerValue() = default;
    virtual std::string Name() = 0;
  };

  // These exist so derived classes can access the private parts of
  // Value, Symbol, String, and Object, which are all friends of Runtime.
  template <typename T>
  static T make(PointerValue* pv);
  static const PointerValue* getPointerValue(const Pointer& pointer);
  static const PointerValue* getPointerValue(const Value& value);

 protected:
  friend class Pointer;
  friend class PropNameID;
  friend class Symbol;
  friend class String;
  friend class Object;
  friend class Array;
  friend class ArrayBuffer;
  friend class Function;
  friend class Value;
  friend class Scope;
  friend class JSError;
  friend class Global;
  friend class BigInt;

  virtual PointerValue* cloneSymbol(const Runtime::PointerValue* pv) = 0;
  virtual PointerValue* cloneString(const Runtime::PointerValue* pv) = 0;
  virtual PointerValue* cloneObject(const Runtime::PointerValue* pv) = 0;
  virtual PointerValue* clonePropNameID(const Runtime::PointerValue* pv) = 0;

  virtual PropNameID createPropNameIDFromAscii(const char* str,
                                               size_t length) = 0;
  virtual PropNameID createPropNameIDFromUtf8(const uint8_t* utf8,
                                              size_t length) = 0;
  virtual PropNameID createPropNameIDFromString(const String& str) = 0;
  virtual std::string utf8(const PropNameID&) = 0;
  virtual bool compare(const PropNameID&, const PropNameID&) = 0;

  virtual std::optional<std::string> symbolToString(const Symbol&) = 0;

  virtual String createStringFromAscii(const char* str, size_t length) = 0;
  virtual String createStringFromUtf8(const uint8_t* utf8, size_t length) = 0;
  virtual std::string utf8(const String&) = 0;

  virtual Object createObject() = 0;
  virtual Object createObject(std::shared_ptr<HostObject> ho) = 0;
  virtual std::weak_ptr<HostObject> getHostObject(const piper::Object&) = 0;
  virtual HostFunctionType& getHostFunction(const piper::Function&) = 0;

  virtual std::optional<Value> getProperty(const Object&,
                                           const PropNameID& name) = 0;
  virtual std::optional<Value> getProperty(const Object&,
                                           const String& name) = 0;
  virtual bool hasProperty(const Object&, const PropNameID& name) = 0;
  virtual bool hasProperty(const Object&, const String& name) = 0;
  virtual bool setPropertyValue(Object&, const PropNameID& name,
                                const Value& value) = 0;
  virtual bool setPropertyValue(Object&, const String& name,
                                const Value& value) = 0;

  virtual bool isArray(const Object&) const = 0;
  virtual bool isArrayBuffer(const Object&) const = 0;
  virtual bool isFunction(const Object&) const = 0;
  virtual bool isHostObject(const piper::Object&) const = 0;
  virtual bool isHostFunction(const piper::Function&) const = 0;
  virtual std::optional<Array> getPropertyNames(const Object&) = 0;

  virtual std::optional<Array> createArray(size_t length) = 0;
  virtual ArrayBuffer createArrayBufferCopy(const uint8_t* bytes,
                                            size_t byte_length) = 0;
  virtual ArrayBuffer createArrayBufferNoCopy(
      std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) = 0;
  virtual std::optional<BigInt> createBigInt(const std::string& value,
                                             Runtime& rt) = 0;
  virtual std::optional<size_t> size(const Array&) = 0;
  virtual size_t size(const ArrayBuffer&) = 0;
  virtual uint8_t* data(const ArrayBuffer&) = 0;
  virtual size_t copyData(const ArrayBuffer&, uint8_t*, size_t) = 0;
  virtual std::optional<Value> getValueAtIndex(const Array&, size_t i) = 0;
  virtual bool setValueAtIndexImpl(Array&, size_t i, const Value& value) = 0;

  virtual Function createFunctionFromHostFunction(const PropNameID& name,
                                                  unsigned int paramCount,
                                                  HostFunctionType func) = 0;
  virtual std::optional<Value> call(const Function&, const Value& jsThis,
                                    const Value* args, size_t count) = 0;
  virtual std::optional<Value> callAsConstructor(const Function&,
                                                 const Value* args,
                                                 size_t count) = 0;
  void ClearHostContainers();

  // Private data for managing scopes.
  class ScopeState {
   public:
    virtual ~ScopeState() {}
  };
  virtual ScopeState* pushScope();
  virtual void popScope(ScopeState*);

  virtual bool strictEquals(const Symbol& a, const Symbol& b) const = 0;
  virtual bool strictEquals(const String& a, const String& b) const = 0;
  virtual bool strictEquals(const Object& a, const Object& b) const = 0;

  virtual bool instanceOf(const Object& o, const Function& f) = 0;

  // static std::atomic<intptr_t> g_counter_;

  template <typename Plain, typename Base>
  friend class RuntimeDecorator;

 protected:
  enum class CircularCheckFlags {
    UNSET = 0,
    DISABLE = 1,
    ENABLE = 2,
  };
  JSRuntimeCreatedType created_type_;
  int64_t runtime_id_;
  std::string group_id_;
  std::shared_ptr<bool> is_runtime_destroyed_ = std::make_shared<bool>(false);
  // This field is protected since it is written in InitRuntime
  std::shared_ptr<JSIExceptionHandler> exception_handler_;
  // Whether is in JSError construction processing. If another JSError happen
  // when construct a JSError, it may enter a dead loop. Using the flag to avoid
  // dead loop.
  bool is_in_js_error_construction_processing{false};

  CircularCheckFlags circular_data_check_flag_{CircularCheckFlags::UNSET};

  bool enable_user_bytecode_ = false;
  std::string bytecode_source_url_;  // url of template.js file
  bool enable_js_binding_api_throw_exception_{false};
  bool gc_flag_{false};
  std::unique_ptr<BytecodeGetter> bytecode_getter_;

 private:
  std::unordered_map<HostObject*, std::shared_ptr<HostObject>>
      host_object_containers_;
  std::unordered_map<HostFunctionType*, std::shared_ptr<HostFunctionType>>
      host_function_containers_;
};

// GCPauseSuppressionMode is used for performance. In GCPauseSuppressionMode
// scope, GC will be disabled if the js_heap is small.
class GCPauseSuppressionMode {
 public:
  GCPauseSuppressionMode(Runtime* rt) : runtime_(rt) {
    origin_mode_ = runtime_->GetGCPauseSuppressionMode();
    runtime_->SetGCPauseSuppressionMode(true);
  }
  ~GCPauseSuppressionMode() {
    runtime_->SetGCPauseSuppressionMode(origin_mode_);
  }

 private:
  Runtime* runtime_;
  bool origin_mode_;
};

// Base class for pointer-storing types.
class Pointer {
 protected:
  explicit Pointer(Pointer&& other) : ptr_(other.ptr_) { other.ptr_ = nullptr; }

  ~Pointer() {
    if (ptr_) {
      ptr_->invalidate();
    }
  }

  Pointer& operator=(Pointer&& other);

  friend class Runtime;
  friend class Value;

  explicit Pointer(Runtime::PointerValue* ptr) : ptr_(ptr) {}

  typename Runtime::PointerValue* ptr_;
};

/// Represents something that can be a JS property key.  Movable, not copyable.
class PropNameID : public Pointer {
 public:
  using Pointer::Pointer;

  PropNameID(Runtime& runtime, const PropNameID& other)
      : PropNameID(runtime.clonePropNameID(other.ptr_)) {}

  PropNameID(PropNameID&& other) = default;
  PropNameID& operator=(PropNameID&& other) = default;

  /// Create a JS property name id from ascii values.  The data is
  /// copied.
  static PropNameID forAscii(Runtime& runtime, const char* str, size_t length) {
    return runtime.createPropNameIDFromAscii(str, length);
  }

  /// Create a property name id from a nul-terminated C ascii name.  The data is
  /// copied.
  static PropNameID forAscii(Runtime& runtime, const char* str) {
    return forAscii(runtime, str, strlen(str));
  }

  /// Create a PropNameID from a C++ string. The string is copied.
  static PropNameID forAscii(Runtime& runtime, const std::string& str) {
    return forAscii(runtime, str.c_str(), str.size());
  }

  /// Create a PropNameID from utf8 values.  The data is copied.
  static PropNameID forUtf8(Runtime& runtime, const uint8_t* utf8,
                            size_t length) {
    return runtime.createPropNameIDFromUtf8(utf8, length);
  }

  /// Create a PropNameID from utf8-encoded octets stored in a
  /// std::string.  The string data is transformed and copied.
  static PropNameID forUtf8(Runtime& runtime, const std::string& utf8) {
    return runtime.createPropNameIDFromUtf8(
        reinterpret_cast<const uint8_t*>(utf8.data()), utf8.size());
  }

  /// Create a PropNameID from a JS string.
  static PropNameID forString(Runtime& runtime, const piper::String& str) {
    return runtime.createPropNameIDFromString(str);
  }

  // Creates a vector of PropNameIDs constructed from given arguments.
  template <typename... Args>
  static std::vector<PropNameID> names(Runtime& runtime, Args&&... args);

  // Creates a vector of given PropNameIDs.
  template <size_t N>
  static std::vector<PropNameID> names(PropNameID (&&propertyNames)[N]);

  /// Copies the data in a PropNameID as utf8 into a C++ string.
  std::string utf8(Runtime& runtime) const { return runtime.utf8(*this); }

  static bool compare(Runtime& runtime, const piper::PropNameID& a,
                      const piper::PropNameID& b) {
    return runtime.compare(a, b);
  }

  friend class Runtime;
  friend class Value;
};

/// Represents a JS Symbol (es6).  Movable, not copyable.
/// TODO T40778724: this is a limited implementation sufficient for
/// the debugger not to crash when a Symbol is a property in an Object
/// or element in an array.  Complete support for creating will come
/// later.
class Symbol : public Pointer {
 public:
  using Pointer::Pointer;

  Symbol(Symbol&& other) = default;
  Symbol& operator=(Symbol&& other) = default;

  /// \return whether a and b refer to the same symbol.
  static bool strictEquals(Runtime& runtime, const Symbol& a, const Symbol& b) {
    return runtime.strictEquals(a, b);
  }

  /// Converts a Symbol into a C++ string as JS .toString would.  The output
  /// will look like \c Symbol(description) .
  std::optional<std::string> toString(Runtime& runtime) const {
    return runtime.symbolToString(*this);
  }

  friend class Runtime;
  friend class Value;
};

/// Represents a JS String.  Movable, not copyable.
class String : public Pointer {
 public:
  using Pointer::Pointer;

  String(String&& other) = default;
  String& operator=(String&& other) = default;

  /// Create a JS string from ascii values.  The string data is
  /// copied.
  static String createFromAscii(Runtime& runtime, const char* str,
                                size_t length) {
    return runtime.createStringFromAscii(str, length);
  }

  /// Create a JS string from a nul-terminated C ascii string.  The
  /// string data is copied.
  static String createFromAscii(Runtime& runtime, const char* str) {
    return createFromAscii(runtime, str, strlen(str));
  }

  /// Create a JS string from a C++ string.  The string data is
  /// copied.
  static String createFromAscii(Runtime& runtime, const std::string& str) {
    return createFromAscii(runtime, str.c_str(), str.size());
  }

  /// Create a JS string from utf8-encoded octets.  The string data is
  /// transformed and copied.
  static String createFromUtf8(Runtime& runtime, const uint8_t* utf8,
                               size_t length) {
    return runtime.createStringFromUtf8(utf8, length);
  }

  /// Create a JS string from utf8-encoded octets stored in a
  /// std::string.  The string data is transformed and copied.
  static String createFromUtf8(Runtime& runtime, const std::string& utf8) {
    return runtime.createStringFromUtf8(
        reinterpret_cast<const uint8_t*>(utf8.data()), utf8.length());
  }

  /// \return whether a and b contain the same characters.
  static bool strictEquals(Runtime& runtime, const String& a, const String& b) {
    return runtime.strictEquals(a, b);
  }

  /// Copies the data in a JS string as utf8 into a C++ string.
  std::string utf8(Runtime& runtime) const { return runtime.utf8(*this); }

  friend class Runtime;
  friend class Value;
};

class Array;
class Function;

/// Represents a JS Object.  Movable, not copyable.
class Object : public Pointer {
 public:
  using Pointer::Pointer;

  Object(Object&& other) = default;
  Object& operator=(Object&& other) = default;

  /// Creates a new Object instance, like '{}' in JS.
  Object(Runtime& runtime) : Object(runtime.createObject()) {}

  static Object createFromHostObject(Runtime& runtime,
                                     std::shared_ptr<HostObject> ho) {
    return runtime.createObject(std::move(ho));
  }

  /// \return whether this and \c obj are the same JSObject or not.
  static bool strictEquals(Runtime& runtime, const Object& a, const Object& b) {
    return runtime.strictEquals(a, b);
  }

  /// \return the result of `this instanceOf ctor` in JS.
  bool instanceOf(Runtime& rt, const Function& ctor) {
    return rt.instanceOf(*this, ctor);
  }

  /// \return the property of the object with the given ascii name.
  /// If the name isn't a property on the object, returns the
  /// undefined value.
  std::optional<Value> getProperty(Runtime& runtime, const char* name) const;

  /// \return the property of the object with the String name.
  /// If the name isn't a property on the object, returns the
  /// undefined value.
  std::optional<Value> getProperty(Runtime& runtime, const String& name) const;

  /// \return the property of the object with the given JS PropNameID
  /// name.  If the name isn't a property on the object, returns the
  /// undefined value.
  std::optional<Value> getProperty(Runtime& runtime,
                                   const PropNameID& name) const;

  /// \return true if and only if the object has a property with the
  /// given ascii name.
  bool hasProperty(Runtime& runtime, const char* name) const;

  /// \return true if and only if the object has a property with the
  /// given String name.
  bool hasProperty(Runtime& runtime, const String& name) const;

  /// \return true if and only if the object has a property with the
  /// given PropNameID name.
  bool hasProperty(Runtime& runtime, const PropNameID& name) const;

  /// Sets the property value from a Value or anything which can be
  /// used to make one: nullptr_t, bool, double, int, const char*,
  /// String, or Object.
  template <typename T>
  bool setProperty(Runtime& runtime, const char* name, T&& value);

  /// Sets the property value from a Value or anything which can be
  /// used to make one: nullptr_t, bool, double, int, const char*,
  /// String, or Object.
  template <typename T>
  bool setProperty(Runtime& runtime, const String& name, T&& value);

  /// Sets the property value from a Value or anything which can be
  /// used to make one: nullptr_t, bool, double, int, const char*,
  /// String, or Object.
  template <typename T>
  bool setProperty(Runtime& runtime, const PropNameID& name, T&& value);

  /// \return true iff JS \c Array.isArray() would return \c true.  If
  /// so, then \c getArray() will succeed.
  bool isArray(Runtime& runtime) const { return runtime.isArray(*this); }

  /// \return true iff the Object is an ArrayBuffer. If so, then \c
  /// getArrayBuffer() will succeed.
  bool isArrayBuffer(Runtime& runtime) const {
    return runtime.isArrayBuffer(*this);
  }

  /// \return true iff the Object is callable.  If so, then \c
  /// getFunction will succeed.
  bool isFunction(Runtime& runtime) const { return runtime.isFunction(*this); }

  /// \return true iff the Object was initialized with \c createFromHostObject
  /// and the HostObject passed is of type \c T. If returns \c true then
  /// \c getHostObject<T> will succeed.
  template <typename T = HostObject>
  bool isHostObject(Runtime& runtime) const;

  /// \return an Array instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will assert.
  Array getArray(Runtime& runtime) const&;

  /// \return an Array instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will assert.
  Array getArray(Runtime& runtime) &&;

  /// \return an BigInt instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will assert.
  BigInt getBigInt(Runtime& runtime) const&;

  /// \return an BigInt instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will assert.
  BigInt getBigInt(Runtime& runtime) &&;

  /// \return an Array instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will throw
  /// JSIException.
  std::optional<Array> asArray(Runtime& runtime) const&;

  /// \return an Array instance which refers to the same underlying
  /// object.  If \c isArray() would return false, this will throw
  /// JSIException.
  std::optional<Array> asArray(Runtime& runtime) &&;

  /// \return an ArrayBuffer instance which refers to the same underlying
  /// object.  If \c isArrayBuffer() would return false, this will assert.
  ArrayBuffer getArrayBuffer(Runtime& runtime) const&;

  /// \return an ArrayBuffer instance which refers to the same underlying
  /// object.  If \c isArrayBuffer() would return false, this will assert.
  ArrayBuffer getArrayBuffer(Runtime& runtime) &&;

  /// \return a Function instance which refers to the same underlying
  /// object.  If \c isFunction() would return false, this will assert.
  Function getFunction(Runtime& runtime) const&;

  /// \return a Function instance which refers to the same underlying
  /// object.  If \c isFunction() would return false, this will assert.
  Function getFunction(Runtime& runtime) &&;

  /// \return a Function instance which refers to the same underlying
  /// object.  If \c isFunction() would return false, this will throw
  /// JSIException.
  std::optional<Function> asFunction(Runtime& runtime) const&;

  /// \return a Function instance which refers to the same underlying
  /// object.  If \c isFunction() would return false, this will throw
  /// JSIException.
  std::optional<Function> asFunction(Runtime& runtime) &&;

  /// \return a shared_ptr<T> which refers to the same underlying
  /// \c HostObject that was used to create this object. If \c isHostObject<T>
  /// is false, this will assert. Note that this does a type check and will
  /// assert if the underlying HostObject isn't of type \c T
  template <typename T = HostObject>
  std::weak_ptr<T> getHostObject(Runtime& runtime) const;

  /// \return same as \c getProperty(name).asObject(), except with
  /// a better exception message.
  std::optional<Object> getPropertyAsObject(Runtime& runtime,
                                            const char* name) const;

  /// \return similar to \c
  /// getProperty(name).getObject().getFunction(), except it will
  /// throw JSIException instead of asserting if the property is
  /// not an object, or the object is not callable.
  BASE_EXPORT std::optional<Function> getPropertyAsFunction(
      Runtime& runtime, const char* name) const;

  /// \return an Array consisting of all enumerable property names in
  /// the object and its prototype chain.  All values in the return
  /// will be isString().  (This is probably not optimal, but it
  /// works.  I only need it in one place.)
  std::optional<Array> getPropertyNames(Runtime& runtime) const;

 protected:
  bool setPropertyValue(Runtime& runtime, const String& name,
                        const Value& value) {
    return runtime.setPropertyValue(*this, name, value);
  }

  bool setPropertyValue(Runtime& runtime, const PropNameID& name,
                        const Value& value) {
    return runtime.setPropertyValue(*this, name, value);
  }

  friend class Runtime;
  friend class Value;
};

/// Represents a JS Object which can be efficiently used as an array
/// with integral indices.
class Array : public Object {
 public:
  Array(Array&&) = default;

  // create a new Array instance
  static std::optional<Array> createWithLength(Runtime& runtime,
                                               size_t length) {
    return runtime.createArray(length);
  }

  Array& operator=(Array&&) = default;

  /// \return the size of the Array, according to its length property.
  /// (C++ naming convention)
  std::optional<size_t> size(Runtime& runtime) const {
    return runtime.size(*this);
  }

  /// \return the size of the Array, according to its length property.
  /// (JS naming convention)
  std::optional<size_t> length(Runtime& runtime) const { return size(runtime); }

  /// \return the property of the array at index \c i.  If there is no
  /// such property, returns the undefined value.  If \c i is out of
  /// range [ 0..\c length ] throws a JSIException.
  std::optional<Value> getValueAtIndex(Runtime& runtime, size_t i) const;

  /// Sets the property of the array at index \c i.  The argument
  /// value behaves as with Object::setProperty().  If \c i is out of
  /// range [ 0..\c length ] throws a JSIException.
  template <typename T>
  bool setValueAtIndex(Runtime& runtime, size_t i, T&& value);

  /// There is no current API for changing the size of an array once
  /// created.  We'll probably need that eventually.

 private:
  friend class Object;
  friend class Value;

  bool setValueAtIndexImpl(Runtime& runtime, size_t i, const Value& value) {
    return runtime.setValueAtIndexImpl(*this, i, value);
  }

  Array(Runtime::PointerValue* value) : Object(value) {}
};

/// Represents a JSArrayBuffer
class ArrayBuffer : public Object {
 public:
  ArrayBuffer(ArrayBuffer&&) = default;
  ArrayBuffer& operator=(ArrayBuffer&&) = default;
  ArrayBuffer(Runtime& runtime, const uint8_t* bytes, size_t byte_length)
      : ArrayBuffer(runtime.createArrayBufferCopy(bytes, byte_length)) {}
  ArrayBuffer(Runtime& runtime, std::unique_ptr<const uint8_t[]> bytes,
              size_t byte_length)
      : ArrayBuffer(
            runtime.createArrayBufferNoCopy(std::move(bytes), byte_length)) {}
  /// Creates a new empty ArrayBuffer instance.
  ArrayBuffer(Runtime& runtime) : Object(runtime) {}
  /// \return the size of the ArrayBuffer, according to its byteLength property.
  /// (C++ naming convention)
  size_t size(Runtime& runtime) const { return runtime.size(*this); }

  size_t length(Runtime& runtime) const { return runtime.size(*this); }

  uint8_t* data(Runtime& runtime) const { return runtime.data(*this); }

  size_t copyData(Runtime& runtime, uint8_t* dest_buf, size_t dest_len) const {
    return runtime.copyData(*this, dest_buf, dest_len);
  }

 private:
  friend class Object;
  friend class Value;

  ArrayBuffer(Runtime::PointerValue* value) : Object(value) {}
};

/// Represents a JS Object which is guaranteed to be Callable.
class Function : public Object {
 public:
  Function(Function&&) = default;
  Function& operator=(Function&&) = default;

  /// Create a function which, when invoked, calls C++ code. If the
  /// function throws an exception, a JS Error will be created and
  /// thrown.
  /// \param name the name property for the function.
  /// \param paramCount the length property for the function, which
  /// may not be the number of arguments the function is passed.
  static Function createFromHostFunction(Runtime& runtime,
                                         const piper::PropNameID& name,
                                         unsigned int paramCount,
                                         piper::HostFunctionType func);

  /// Calls the function with \c count \c args.  The \c this value of
  /// the JS function will be undefined.
  std::optional<Value> call(Runtime& runtime, const Value* args,
                            size_t count) const;

  /// Calls the function with a \c std::initializer_list of Value
  /// arguments. The \c this value of the JS function will be
  /// undefined.
  std::optional<Value> call(Runtime& runtime,
                            std::initializer_list<Value> args) const;

  /// Calls the function with any number of arguments similarly to
  /// Object::setProperty().  The \c this value of the JS function
  /// will be undefined.
  template <typename... Args>
  std::optional<Value> call(Runtime& runtime, Args&&... args) const;

  /// Calls the function with \c count \c args and \c jsThis value passed
  /// as this value.
  std::optional<Value> callWithThis(Runtime& Runtime, const Object& jsThis,
                                    const Value* args, size_t count) const;

  /// Calls the function with a \c std::initializer_list of Value
  /// arguments. The \c this value of the JS function will be
  /// undefined.
  std::optional<Value> callWithThis(Runtime& runtime, const Object& jsThis,
                                    std::initializer_list<Value> args) const;

  /// Calls the function as a constructor with \c count \c args. Equivalent
  /// to calling `new Func` where `Func` is the js function reqresented by
  /// this.
  std::optional<Value> callAsConstructor(Runtime& runtime, const Value* args,
                                         size_t count) const;

  /// Same as above `callAsConstructor`, except use an initializer_list to
  /// supply the arguments.
  std::optional<Value> callAsConstructor(
      Runtime& runtime, std::initializer_list<Value> args) const;

  /// Same as above `callAsConstructor`, but automatically converts/wraps
  /// any argument with a jsi Value.
  template <typename... Args>
  std::optional<Value> callAsConstructor(Runtime& runtime,
                                         Args&&... args) const;

  /// Returns whether this was created with Function::createFromHostFunction.
  /// If true then you can use getHostFunction to get the underlying
  /// HostFunctionType.
  bool isHostFunction(Runtime& runtime) const {
    return runtime.isHostFunction(*this);
  }

  /// Returns the underlying HostFunctionType iff isHostFunction returns true
  /// and asserts otherwise. You can use this to use std::function<>::target
  /// to get the object that was passed to create the HostFunctionType.
  ///
  /// Note: The reference returned is borrowed from the JS object underlying
  ///       \c this, and thus only lasts as long as the object underlying
  ///       \c this does.
  HostFunctionType& getHostFunction(Runtime& runtime) const {
    DCHECK(isHostFunction(runtime));
    return runtime.getHostFunction(*this);
  }

 private:
  /// Calls the function with any number of arguments similarly to

  friend class Object;
  friend class Value;

  Function(Runtime::PointerValue* value) : Object(value) {}
};

/// Represents any JS Value (undefined, null, boolean, number, symbol,
/// string, or object).  Movable, or explicitly copyable (has no copy
/// ctor).
class Value {
 public:
  enum class ValueKind {
    UndefinedKind,
    NullKind,
    BooleanKind,
    NumberKind,
    SymbolKind,
    StringKind,
    ObjectKind,
    PointerKind = SymbolKind,
  };
  /// Default ctor creates an \c undefined JS value.
  Value() : Value(ValueKind::UndefinedKind) {}

  /// Creates a \c null JS value.
  /* implicit */ Value(std::nullptr_t) : kind_(ValueKind::NullKind) {}

  /// Creates a boolean JS value.
  /* implicit */ Value(bool b) : Value(ValueKind::BooleanKind) {
    data_.boolean = b;
  }

  /// Creates a number JS value.
  /* implicit */ Value(double d) : Value(ValueKind::NumberKind) {
    data_.number = d;
  }

  /// Creates a number JS value.
  /* implicit */ Value(int i) : Value(ValueKind::NumberKind) {
    data_.number = i;
  }

  /// Moves a Symbol, String, or Object rvalue into a new JS value.
  template <typename T>
  /* implicit */ Value(T&& other) : Value(kindOf(other)) {
    /*  static_assert(std::is_base_of<Symbol, T>::value ||
                        std::is_base_of<String, T>::value ||
                        std::is_base_of<Object, T>::value,
                    "Value cannot be implicitly move-constructed from this
       type");*/
    typedef typename std::remove_reference<T>::type T1;
    new (&data_.pointer) T1(std::move(other));
  }

  /// Value("foo") will treat foo as a bool.  This makes doing that a
  /// compile error.
  template <typename T = void>
  Value(const char*) {
    static_assert(!std::is_same<void, T>::value,
                  "Value cannot be constructed directly from const char*");
  }

  BASE_EXPORT Value(Value&& value);

  /// Copies a Symbol lvalue into a new JS value.
  Value(Runtime& runtime, const Symbol& sym) : Value(ValueKind::SymbolKind) {
    new (&data_.pointer) String(runtime.cloneSymbol(sym.ptr_));
  }

  /// Copies a String lvalue into a new JS value.
  Value(Runtime& runtime, const String& str) : Value(ValueKind::StringKind) {
    new (&data_.pointer) String(runtime.cloneString(str.ptr_));
  }

  /// Copies a Object lvalue into a new JS value.
  Value(Runtime& runtime, const Object& obj) : Value(ValueKind::ObjectKind) {
    new (&data_.pointer) Object(runtime.cloneObject(obj.ptr_));
  }

  /// Creates a JS value from another Value lvalue.
  Value(Runtime& runtime, const Value& value);

  /// Value(rt, "foo") will treat foo as a bool.  This makes doing
  /// that a compile error.
  template <typename T = void>
  Value(Runtime&, const char*) {
    static_assert(!std::is_same<T, void>::value,
                  "Value cannot be constructed directly from const char*");
  }

  BASE_EXPORT ~Value();
  // \return the undefined \c Value.
  static Value undefined() { return Value(); }

  // \return the null \c Value.
  static Value null() { return Value(nullptr); }

  static std::optional<Value> createFromJsonString(Runtime& runtime,
                                                   String& string);

  // \return a \c Value created from a utf8-encoded JSON string.
  static std::optional<Value> createFromJsonUtf8(Runtime& runtime,
                                                 const uint8_t* json,
                                                 size_t length);

  /// \return according to the SameValue algorithm see more here:
  //  https://www.ecma-international.org/ecma-262/5.1/#sec-11.9.4
  static bool strictEquals(Runtime& runtime, const Value& a, const Value& b);

  Value& operator=(Value&& other) {
    this->~Value();
    new (this) Value(std::move(other));
    return *this;
  }

  bool isUndefined() const { return kind_ == ValueKind::UndefinedKind; }

  bool isNull() const { return kind_ == ValueKind::NullKind; }

  bool isBool() const { return kind_ == ValueKind::BooleanKind; }

  bool isNumber() const { return kind_ == ValueKind::NumberKind; }

  bool isString() const { return kind_ == ValueKind::StringKind; }

  bool isSymbol() const { return kind_ == ValueKind::SymbolKind; }

  bool isObject() const { return kind_ == ValueKind::ObjectKind; }
  ValueKind kind() const { return kind_; }

  /// \return the boolean value, or asserts if not a boolean.
  bool getBool() const {
    DCHECK(isBool());
    return data_.boolean;
  }

  /// \return the number value, or asserts if not a number.
  double getNumber() const {
    DCHECK(isNumber());
    return data_.number;
  }

  /// \return the number value, or throws JSIException if not a
  /// number.
  std::optional<double> asNumber(Runtime& rt) const;

  /// \return the Symbol value, or asserts if not a symbol.
  Symbol getSymbol(Runtime& runtime) const& {
    DCHECK(isSymbol());
    return Symbol(runtime.cloneSymbol(data_.pointer.ptr_));
  }

  /// \return the Symbol value, or asserts if not a symbol.
  /// Can be used on rvalue references to avoid cloning more symbols.
  Symbol getSymbol(Runtime&) && {
    DCHECK(isSymbol());
    auto ptr = data_.pointer.ptr_;
    data_.pointer.ptr_ = nullptr;
    return static_cast<Symbol>(ptr);
  }

  /// \return the Symbol value, or throws JSIException if not a
  /// symbol
  std::optional<Symbol> asSymbol(Runtime& runtime) const&;
  std::optional<Symbol> asSymbol(Runtime& runtime) &&;

  /// \return the String value, or asserts if not a string.
  String getString(Runtime& runtime) const& {
    DCHECK(isString());
    return String(runtime.cloneString(data_.pointer.ptr_));
  }

  /// \return the String value, or asserts if not a string.
  /// Can be used on rvalue references to avoid cloning more strings.
  String getString(Runtime&) && {
    DCHECK(isString());
    auto ptr = data_.pointer.ptr_;
    data_.pointer.ptr_ = nullptr;
    return static_cast<String>(ptr);
  }

  /// \return the String value, or throws JSIException if not a
  /// string.
  BASE_EXPORT std::optional<String> asString(Runtime& runtime) const&;
  std::optional<String> asString(Runtime& runtime) &&;

  /// \return the Object value, or asserts if not an object.
  Object getObject(Runtime& runtime) const& {
    DCHECK(isObject());
    return Object(runtime.cloneObject(data_.pointer.ptr_));
  }

  /// \return the Object value, or asserts if not an object.
  /// Can be used on rvalue references to avoid cloning more objects.
  Object getObject(Runtime&) && {
    DCHECK(isObject());
    auto ptr = data_.pointer.ptr_;
    data_.pointer.ptr_ = nullptr;
    return static_cast<Object>(ptr);
  }

  /// \return the Object value, or throws JSIException if not an
  /// object.
  std::optional<Object> asObject(Runtime& runtime) const&;
  std::optional<Object> asObject(Runtime& runtime) &&;

  // \return a String like JS .toString() would do.
  BASE_EXPORT std::optional<String> toString(Runtime& runtime) const;

  /// \return string of the type of the Value
  std::string typeToString() const;

  std::optional<Value> toJsonString(Runtime& runtime) const;

 private:
  friend class Runtime;

  union Data {
    // Value's ctor and dtor will manage the lifecycle of the contained Data.
    Data() {
      static_assert(sizeof(Data) == sizeof(uint64_t),
                    "Value data should fit in a 64-bit register");
    }
    ~Data() {}

    // scalars
    bool boolean;
    double number;
    // pointers
    Pointer pointer;  // Symbol, String, Object, Array, Function
  };

  Value(ValueKind kind) : kind_(kind) {}

  constexpr static ValueKind kindOf(const Symbol&) {
    return ValueKind::SymbolKind;
  }
  constexpr static ValueKind kindOf(const String&) {
    return ValueKind::StringKind;
  }
  constexpr static ValueKind kindOf(const Object&) {
    return ValueKind::ObjectKind;
  }

  ValueKind kind_;
  Data data_;

  // In the future: Value becomes NaN-boxed. See T40538354.
};

/// represents a JS Object which can be efficiently used as an BigInt object
class BigInt : public Object {
 public:
  BigInt(BigInt&&) = default;
  BigInt& operator=(BigInt&&) = default;

  // create a new BigInt instance from const std::string
  static std::optional<BigInt> createWithString(Runtime& runtime,
                                                const std::string& value);

 private:
  friend class Object;
  friend class Value;
  BigInt(Runtime::PointerValue* value) : Object(value) {}
};

/// Not movable and not copyable RAII marker advising the underlying
/// JavaScript VM to track resources allocated since creation until
/// destruction so that they can be recycled eagerly when the Scope
/// goes out of scope instead of floating in the air until the next
/// garbage collection or any other delayed release occurs.
///
/// This API should be treated only as advice, implementations can
/// choose to ignore the fact that Scopes are created or destroyed.
///
/// This class is an exception to the rule allowing destructors to be
/// called without proper synchronization (see Runtime documentation).
/// The whole point of this class is to enable all sorts of clean ups
/// when the destructor is called and this proper synchronization is
/// required at that time.
///
/// Instances of this class are intended to be created as automatic stack
/// variables in which case destructor calls don't require any additional
/// locking, provided that the lock (if any) is managed with RAII helpers.
class Scope {
 public:
  explicit Scope(Runtime& rt) : rt_(rt), prv_(rt.pushScope()) {}
  ~Scope() { rt_.popScope(prv_); };

  Scope(const Scope&) = delete;
  Scope(Scope&&) = delete;

  Scope& operator=(const Scope&) = delete;
  Scope& operator=(Scope&&) = delete;

  template <typename F>
  static auto callInNewScope(Runtime& rt, F f) -> decltype(f()) {
    Scope s(rt);
    return f();
  }

 private:
  Runtime& rt_;
  Runtime::ScopeState* prv_;
};

/// Base class for jsi exceptions
class JSIException {
 protected:
  JSIException() = default;
  virtual ~JSIException() = default;

  explicit JSIException(std::string message)
      : JSIException(std::move(message), {}, error::E_BTS_RUNTIME_ERROR) {}

  JSIException(std::string message, std::string stack, int32_t error_code)
      : message_(std::move(message)),
        stack_(std::move(stack)),
        error_code_(error_code){};

 public:
  virtual const char* name() const = 0;
  const std::string& message() const { return message_; }
  const std::string& stack() const { return stack_; }
  int32_t errorCode() const { return error_code_; }

  std::string ToString() const {
    return std::string("{name:") + name() + ";message:" + message() +
           ";error_code:" + std::to_string(errorCode()) + ";stack:" + stack() +
           ";}";
  }

 protected:
  std::string message_;
  std::string stack_;
  int32_t error_code_{error::E_BTS_RUNTIME_ERROR};
};

/// This exception will be thrown by API functions on errors not related to
/// JavaScript execution.
class JSINativeException : public JSIException {
 public:
  JSINativeException() = default;
  ~JSINativeException() override = default;
  JSINativeException(const std::string name, const std::string message,
                     const std::string stack, bool is_js_error,
                     int32_t error_code)
      : JSIException(std::move(message), std::move(stack), error_code),
        name_(std::move(name)),
        is_js_error_(is_js_error) {}
  JSINativeException(std::string message, std::string function_name,
                     std::string file_name, int line_number)
      : JSIException(std::move(message),
                     ToStack(std::move(function_name), std::move(file_name),
                             line_number),
                     error::E_BTS_RUNTIME_ERROR_BINDINGS_ERROR) {}

  const char* name() const override { return name_.c_str(); }

  void AddStack(std::string function_name, std::string file_name,
                int line_number) {
    stack_ += "\n" + ToStack(std::move(function_name), std::move(file_name),
                             line_number);
  }

  void SetMessage(std::string message) { message_ = message; }
  bool IsJSError() const { return is_js_error_; }

 private:
  std::string ToStack(std::string function_name, std::string file_name,
                      int line_number) {
    return "at " + function_name + "(" + file_name + ": " +
           std::to_string(line_number) + ")";
  }
  std::string name_{"NativeError"};
  bool is_js_error_{false};
};

class JSINativeExceptionCollector {
 public:
  class Scope {
   public:
    explicit Scope() {
      JSINativeExceptionCollector::Instance()->stack_.push({});
    }

    ~Scope() { JSINativeExceptionCollector::Instance()->stack_.pop(); }

    Scope(const Scope& s) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;
  };

  static JSINativeExceptionCollector* Instance() {
    static thread_local JSINativeExceptionCollector instance_;
    return &instance_;
  }

  void SetMessage(std::string message) {
    if (!stack_.top()) {
      stack_.top() = JSINativeException();
    }

    stack_.top()->SetMessage(std::move(message));
  }

  void AddStack(std::string function_name, std::string file_name,
                int line_number) {
    if (!stack_.top()) {
      stack_.top() = JSINativeException();
    }

    stack_.top()->AddStack(std::move(function_name), std::move(file_name),
                           line_number);
  }

  const std::optional<JSINativeException>& GetException() {
    return stack_.top();
  }

  JSINativeExceptionCollector(const JSINativeExceptionCollector&) = delete;
  JSINativeExceptionCollector& operator=(const JSINativeExceptionCollector&) =
      delete;
  JSINativeExceptionCollector(JSINativeExceptionCollector&&) = delete;
  JSINativeExceptionCollector& operator=(JSINativeExceptionCollector&&) =
      delete;

 private:
  JSINativeExceptionCollector(){};
  base::InlineStack<std::optional<JSINativeException>, 8> stack_;
};

/// This exception will be thrown by API functions whenever a JS
/// operation causes an exception as described by the spec, or as
/// otherwise described.
class BASE_EXPORT JSError : public JSIException {
 public:
  class Scope {
   public:
    explicit Scope(Runtime& rt) : rt_(rt) {
      rt.SetInJSErrorConstructionProcessing(true);
    }
    ~Scope() { rt_.SetInJSErrorConstructionProcessing(false); };

    Scope(const Scope&) = delete;
    Scope(Scope&&) = delete;

    Scope& operator=(const Scope&) = delete;
    Scope& operator=(Scope&&) = delete;

   private:
    Runtime& rt_;
  };

  ~JSError() override = default;
  /// Creates a JSError referring to provided \c value
  JSError(Runtime& r, Value&& value);

  /// Creates a JSError referring to new \c Error instance capturing current
  /// JavaScript stack. The error message property is set to given \c message.
  JSError(Runtime& rt, std::string message);

  /// Creates a JSError referring to new \c Error instance capturing current
  /// JavaScript stack. The error message property is set to given \c message.
  JSError(Runtime& rt, const char* message)
      : JSError(rt, std::string(message)){};

  /// Creates a JSError referring to a JavaScript Object having message and
  /// stack properties set to provided values.
  BASE_EXPORT JSError(Runtime& rt, std::string message, std::string stack);

  JSError(std::string message, std::string stack);

  /// Creates a JSError referring to provided value and what string
  /// set to provided message.  This argument order is a bit weird,
  /// but necessary to avoid ambiguity with the above.
  JSError(Runtime& rt, Value&& value, std::string what);

  const piper::Value& value() const {
    DCHECK(value_);
    return *value_;
  }

  const char* name() const override { return name_.c_str(); }

 private:
  // This initializes the value_ member and does some other
  // validation, so it must be called by every branch through the
  // constructors.
  void setValue(Runtime& rt, Value&& value);

  // This needs to be on the heap, because throw requires the object
  // be copyable, and Value is not.
  std::shared_ptr<piper::Value> value_;

  std::string name_{kDefaultName};

  static constexpr const char* kDefaultName = "Error";
};

class JSIExceptionHandler {
 public:
  virtual ~JSIExceptionHandler() = default;

  virtual void onJSIException(const JSIException& exception) = 0;
  virtual void Destroy() {}
};

typedef void (*report_func)(void* ctx, const char*, int);

class VMInstance {
  // VMInstance createVM(StartupData data);
  // static VMInstance createVM();
 public:
  virtual ~VMInstance() = default;
  virtual JSRuntimeType GetRuntimeType() = 0;
  static void SetReportFunction(report_func func) {
    trig_mem_info_event_ = func;
  }

 protected:
  static report_func trig_mem_info_event_;
};

class HostGlobal {
 public:
  virtual void Init(std::shared_ptr<Runtime>& js_runtime_,
                    std::shared_ptr<ConsoleMessagePostMan>& post_man) = 0;
  virtual void Release() = 0;
  virtual ~HostGlobal() { LOGE("~HostGlobal;"); }
};

class JSIObserver {
 public:
  virtual ~JSIObserver() = default;
  virtual void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) = 0;
};

class JSIContext {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;

    virtual void Def() = 0;
  };
  JSIContext(std::shared_ptr<VMInstance> vm) : vm_(vm), postman_(nullptr) {}
  virtual std::shared_ptr<VMInstance> getVM() { return vm_; }
  virtual ~JSIContext() { LOGE("~JSIContext;"); };

  void SetPostMan(std::shared_ptr<ConsoleMessagePostMan> postman) {
    postman_ = postman;
  }

  void SetReleaseObserver(std::shared_ptr<Observer> observer) {
    observer_ = observer;
  }

  void Release() {
    LOGI("JSIContext Release");
    if (observer_) {
      observer_->Def();
    }
  }

  std::shared_ptr<ConsoleMessagePostMan>& GetPostMan() { return postman_; }

 protected:
  std::shared_ptr<Observer> observer_;
  std::shared_ptr<VMInstance> vm_;
  std::shared_ptr<ConsoleMessagePostMan> postman_;
};

// v8 snapshot data
class StartupData {
 public:
};

}  // namespace piper

}  // namespace lynx

#include "core/runtime/jsi/jsi-inl.h"
#endif  // CORE_RUNTIME_JSI_JSI_H_
