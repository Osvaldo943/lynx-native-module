// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/devtool_message_handler_embedder.h"

namespace lynx {
namespace devtool {

DevtoolMessageHandlerEmbedder::DevtoolMessageHandlerEmbedder(
    const std::shared_ptr<MessageHandler>& handler)
    : handler_(handler) {}

void DevtoolMessageHandlerEmbedder::handle(
    const std::shared_ptr<devtool::MessageSender>& sender,
    const std::string& type, const Json::Value& message) {
  if (handler_) {
    handler_->OnMessage(message.toStyledString());
  }
}

}  // namespace devtool
}  // namespace lynx
