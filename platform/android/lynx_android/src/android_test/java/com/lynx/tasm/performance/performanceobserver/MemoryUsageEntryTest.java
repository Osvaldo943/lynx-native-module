// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.performanceobserver;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class MemoryUsageEntryTest {
  private HashMap<String, Object> props;
  @Before
  public void setUp() {
    props = spy(new HashMap<>());
    HashMap<String, Object> mockDetail = new HashMap<>();
    HashMap<String, Object> mockItem = new HashMap<>();
    mockItem.put("category", "heap");
    mockItem.put("sizeKb", 1024.0);
    mockDetail.put("item1", mockItem);

    when(props.get("sizeKb")).thenReturn(2048.0);
    when(props.get("detail")).thenReturn(mockDetail);
  }

  @Test
  public void testConstructorWithValidValues() {
    MemoryUsageEntry entry = new MemoryUsageEntry(props);

    assertEquals(2048.0, entry.sizeKb, 0.001);
    assertFalse(entry.detail.isEmpty());
    assertEquals(1, entry.detail.size());
    assertTrue(entry.detail.containsKey("item1"));
  }

  @Test
  public void testDefaultValuesWhenFieldsMissing() {
    when(props.get("sizeKb")).thenReturn(null);
    when(props.get("detail")).thenReturn(null);

    MemoryUsageEntry entry = new MemoryUsageEntry(props);

    assertEquals(-1.0, entry.sizeKb, 0.001);
    assertTrue(entry.detail.isEmpty());
  }

  @Test
  public void testNullSafetyForDetailConversion() {
    when(props.get("detail")).thenReturn(null);

    MemoryUsageEntry entry = new MemoryUsageEntry(props);

    assertNotNull(entry.detail);
    assertTrue(entry.detail.isEmpty());
  }

  @Test
  public void testNestedStructureCreation() {
    MemoryUsageEntry entry = new MemoryUsageEntry(props);
    MemoryUsageItem item = entry.detail.get("item1");

    assertNotNull(item);
    assertEquals("heap", item.category);
    assertEquals(1024.0, item.sizeKb, 0.001);
  }
}
