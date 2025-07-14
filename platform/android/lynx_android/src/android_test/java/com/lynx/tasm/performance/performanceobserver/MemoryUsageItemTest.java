// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.performanceobserver;

import static org.junit.Assert.*;

import java.util.HashMap;
import org.junit.Test;

public class MemoryUsageItemTest {
  @Test
  public void testConstructorWithFullProps() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("category", "JS");
    props.put("sizeKb", 1024.5);
    HashMap<String, String> detail = new HashMap<>();
    detail.put("module", "core");
    props.put("detail", detail);

    MemoryUsageItem item = new MemoryUsageItem(props);

    assertEquals("JS", item.category);
    assertEquals(1024.5, item.sizeKb, 0.01);
    assertEquals("core", item.detail.get("module"));
  }

  @Test
  public void testDefaultValues() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("invalidKey", "test");

    MemoryUsageItem item = new MemoryUsageItem(props);

    assertEquals("", item.category);
    assertEquals(-1.0, item.sizeKb, 0.01);
    assertTrue(item.detail.isEmpty());
  }
}
