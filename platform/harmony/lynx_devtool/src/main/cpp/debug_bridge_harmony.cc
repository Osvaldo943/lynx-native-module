// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/debug_bridge_harmony.h"

#include <js_native_api.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"
#include "devtool/embedder/core/debug_bridge_embedder.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace devtool {

napi_value DebugBridgeHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("connectDevtools", ConnectDevtools),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "DebugBridgeHarmony", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "DebugBridgeHarmony", cons);
  return exports;
}

napi_value DebugBridgeHarmony::Constructor(napi_env env,
                                           napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value DebugBridgeHarmony::ConnectDevtools(napi_env env,
                                               napi_callback_info info) {
  bool result = false;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  std::string remote_debug_url = base::NapiUtil::ConvertToString(env, args[0]);
  std::vector<std::string> keys;
  base::NapiUtil::ConvertToArrayString(env, args[1], keys);
  std::vector<std::string> values;
  base::NapiUtil::ConvertToArrayString(env, args[2], values);

  if (keys.size() == values.size()) {
    std::unordered_map<std::string, std::string> app_infos;
    for (size_t i = 0; i < keys.size(); ++i) {
      app_infos[keys[i]] = values[i];
    }
    result =
        DebugBridgeEmbedder::GetInstance().Enable(remote_debug_url, app_infos);
    LOGI("ConnectDevtools result:" << result);
  } else {
    result = false;
    LOGE("ConnectDevtools failed, keys.size != values.size()");
  }

  napi_value value;
  napi_create_int32(env, result, &value);
  return value;
}

}  // namespace devtool
}  // namespace lynx
