// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEVTOOLS_MESSAGE_HANDLER_H_
#define DEVTOOL_EMBEDDER_CORE_DEVTOOLS_MESSAGE_HANDLER_H_

#include <string>

namespace lynx {
namespace devtool {

class MessageHandler {
 public:
  virtual void OnMessage(const std::string& message) = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEVTOOLS_MESSAGE_HANDLER_H_
