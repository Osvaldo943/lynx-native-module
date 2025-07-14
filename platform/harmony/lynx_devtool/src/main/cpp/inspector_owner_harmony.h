// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_HARMONY_H_

#include <node_api.h>

#include <memory>

#include "devtool/embedder/core/inspector_owner_embedder.h"

namespace lynx {
namespace devtool {

class InspectorOwnerHarmony {
 public:
  explicit InspectorOwnerHarmony(LynxDevToolProxy *proxy);

 public:
  static napi_value Init(napi_env env, napi_value exports);

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value Destroy(napi_env env, napi_callback_info info);
  static napi_value GetSessionId(napi_env env, napi_callback_info info);

 private:
  std::shared_ptr<InspectorOwnerEmbedder> owner_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_HARMONY_H_
