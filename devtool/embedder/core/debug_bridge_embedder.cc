// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/debug_bridge_embedder.h"

#include "base/include/log/logging.h"
#include "devtool/embedder/core/env_embedder.h"
#include "devtool/lynx_devtool/config/devtool_config.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"
#include "third_party/debug_router/src/debug_router/Common/debug_router.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

using DebugRouter = debugrouter::common::DebugRouter;

DebugBridgeEmbedder& DebugBridgeEmbedder::GetInstance() {
  static lynx::base::NoDestructor<DebugBridgeEmbedder> instance;
  return *instance;
}

DebugBridgeEmbedder::DebugBridgeEmbedder() : open_card_callback_(nullptr) {
  debug_state_listener_ = std::make_shared<DebugStateListenerEmbedder>();
  Init();
}

void DebugBridgeEmbedder::Init() {
  DebugRouter::GetInstance().AddGlobalHandler(this);
  DebugRouter::GetInstance().AddStateListener(debug_state_listener_);
}

bool DebugBridgeEmbedder::Enable(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& app_infos) {
  if (IsEnabled()) {
    LOGW("Devtools already connected!");
    return true;
  }

  if (DebugRouter::GetInstance().IsValidSchema(url)) {
    SetAppInfo(app_infos);
    return DebugRouter::GetInstance().HandleSchema(url);
  }
  return false;
}

// only be used by automatic test
void DebugBridgeEmbedder::EnableDebugging(const std::string& schema) {
  DebugRouter::GetInstance().HandleSchema(schema);
}

bool DebugBridgeEmbedder::IsEnabled() {
  return DebugRouter::GetInstance().IsConnected();
}

void DebugBridgeEmbedder::SetOpenCardCallback(
    DevtoolsOpenCardCallback callback) {
  open_card_callback_ = callback;
}

void DebugBridgeEmbedder::OpenCard(const std::string& url) {
  if (open_card_callback_) {
    open_card_callback_(url);
  }
}

void DebugBridgeEmbedder::OnMessage(const std::string& message,
                                    const std::string& type) {
  if (!type.compare("SetGlobalSwitch")) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(message, root, false)) {
      return;
    }
    bool valid = root["global_key"].isString() && root["global_value"].isBool();
    if (!valid) {
      return;
    }

    std::string key = root["global_key"].asString();
    bool value = root["global_value"].asBool();

    EnvEmbedder::SetSwitch(key, value);

    DebugRouter::GetInstance().SendDataAsync(message, "SetGlobalSwitch", -1);
  } else {
    HandleStopAtEntry(message, type);
  }
}

void DebugBridgeEmbedder::SetAppInfo(
    const std::unordered_map<std::string, std::string>& app_infos) {
  for (auto it = app_infos.begin(); it != app_infos.end(); it++) {
    DebugRouter::GetInstance().SetAppInfo(it->first, it->second);
  }
}

void DebugBridgeEmbedder::HandleStopAtEntry(const std::string& message,
                                            const std::string& type) {
  if (type != devtool::kTypeGetStopAtEntry &&
      type != devtool::kTypeSetStopAtEntry) {
    return;
  }
  Json::Value content;
  Json::Reader reader;
  if (!reader.parse(message, content, false)) {
    return;
  }
  std::string key = content[devtool::kKeyType].asString();
  if (type == devtool::kTypeGetStopAtEntry) {
    bool result = false;
    if (key == devtool::kKeyMTS) {
      result = devtool::DevToolConfig::ShouldStopAtEntry(true);
    } else if (key == devtool::kKeyBTS || key == devtool::kKeyDefault) {
      result = devtool::DevToolConfig::ShouldStopAtEntry(false);
    }
    content[devtool::kKeyValue] = result;
  } else if (type == devtool::kTypeSetStopAtEntry) {
    bool value = content[devtool::kKeyValue].asBool();
    if (key == devtool::kKeyMTS) {
      lynx::devtool::DevToolConfig::SetStopAtEntry(value, true);
    } else if (key == devtool::kKeyBTS || key == devtool::kKeyDefault) {
      lynx::devtool::DevToolConfig::SetStopAtEntry(value);
    }
  }
  DebugRouter::GetInstance().SendDataAsync(content.toStyledString(), type, -1);
}

}  // namespace devtool
}  // namespace lynx
