// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEVTOOL_MESSAGE_HANDLER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_DEVTOOL_MESSAGE_HANDLER_EMBEDDER_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "devtool/embedder/core/devtools_message_handler.h"

namespace lynx {
namespace devtool {
class DevToolMessageHandler;
class MessageSender;

class DevtoolMessageHandlerEmbedder : public devtool::DevToolMessageHandler {
 public:
  DevtoolMessageHandlerEmbedder(const std::shared_ptr<MessageHandler>& handler);
  ~DevtoolMessageHandlerEmbedder() override = default;
  void handle(const std::shared_ptr<devtool::MessageSender>& sender,
              const std::string& type, const Json::Value& message) override;

 private:
  std::shared_ptr<MessageHandler> handler_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_EMBEDDER_CORE_DEVTOOL_MESSAGE_HANDLER_EMBEDDER_H_
