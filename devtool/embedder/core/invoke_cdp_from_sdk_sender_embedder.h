// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_INVOKE_CDP_FROM_SDK_SENDER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_INVOKE_CDP_FROM_SDK_SENDER_EMBEDDER_H_

#include <functional>
#include <string>
#include <utility>

#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

class InvokeCDPFromSDKSenderEmbedder : public lynx::devtool::MessageSender {
 public:
  explicit InvokeCDPFromSDKSenderEmbedder(
      const std::function<void(const std::string&)>&& callback)
      : callback_(std::move(callback)){};

  void SendMessage(const std::string& type, const Json::Value& msg) override {
    if (callback_) {
      callback_(msg.toStyledString());
    }
  };

  void SendMessage(const std::string& type, const std::string& msg) override {
    if (callback_) {
      callback_(msg);
    }
  };

 private:
  std::function<void(const std::string&)> callback_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_INVOKE_CDP_FROM_SDK_SENDER_EMBEDDER_H_
