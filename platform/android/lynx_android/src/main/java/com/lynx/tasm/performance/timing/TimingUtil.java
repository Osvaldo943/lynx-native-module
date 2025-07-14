// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

public class TimingUtil {
  private static final long initialMillis = System.currentTimeMillis();
  private static final long initialNanos = System.nanoTime();

  /**
   * @brief Returns the current time in microseconds.
   *
   * This method returns the current time in microseconds, which is the number of microseconds
   * that have elapsed since the Unix epoch (1970-01-01 00:00:00 UTC).
   *
   * @return The current time in microseconds.
   */
  public static long currentTimeUs() {
    long elapsedNanos = System.nanoTime() - initialNanos;
    return initialMillis * 1000 + elapsedNanos / 1000;
  }
}
