// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.memory;
/**
 * @brief A FunctionalInterface type that constructs and returns a LynxMemoryRecord instance.
 *
 * This FunctionalInterface type can be used to create instances of LynxMemoryRecord with specified
 * category, size in kilobytes, and detail information.
 */
@FunctionalInterface
public interface IMemoryRecordBuilder {
  MemoryRecord build();
}
