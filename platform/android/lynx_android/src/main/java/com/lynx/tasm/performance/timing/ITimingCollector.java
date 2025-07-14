// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

/**
 * @brief The ITimingCollector defines the interface for timing collection.
 * This protocol is adopted by classes that need to collect and manage timing data.
 */
public interface ITimingCollector {
  /**
   * @brief Marks a timing event with the specified key and pipeline ID.
   * @param key The key that uniquely identifies the timing event.
   * @param pipelineID The optional pipeline ID associated with the timing event.
   */
  public void markTiming(final String key, final String pipelineID);

  /**
   * @brief Sets a timing value with the specified timestamp, key, and pipeline ID.
   * @param timestamp The timestamp of the timing event.
   * @param key The key that uniquely identifies the timing event.
   * @param pipelineID The optional pipeline ID associated with the timing event.
   */
  public void setMsTiming(final String key, final long msTimestamp, final String pipelineID);

  /**
   * @brief Marks the end of the paint timing.
   * This method is used to mark the end of the paint timing, typically when the rendering process
   * is complete.
   */
  public void markPaintEndTimingIfNeeded();
}
