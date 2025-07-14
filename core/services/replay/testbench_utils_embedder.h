// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_TESTBENCH_UTILS_EMBEDDER_H_
#define CORE_SERVICES_REPLAY_TESTBENCH_UTILS_EMBEDDER_H_

#include <string>

namespace lynx {
namespace tasm {
namespace replay {

class TestBenchUtilsEmbedder {
 public:
  TestBenchUtilsEmbedder() = default;
  ~TestBenchUtilsEmbedder() = default;
  static std::string ParserTestBenchRecordData(const std::string& source);
};
}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_TESTBENCH_UTILS_EMBEDDER_H_
