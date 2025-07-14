// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

/**
 * This interface serves as a replacement for {@link java.util.function.Consumer}
 * because {@code java.util.function.Consumer} requires API level 24 or higher,
 * whereas Lynx supports a minimum API level of 16. This custom interface ensures
 * compatibility with older Android versions.
 *
 * @param <T> the type of the input to the operation
 */
@FunctionalInterface
public interface LynxConsumer<T> {
  void accept(T t);
}
