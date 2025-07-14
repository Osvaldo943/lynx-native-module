// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.text.TextPaint;
import org.junit.Test;

public class CustomStyleSpanTest {
  private final TextPaint mTextPaint = new TextPaint();

  @Test
  public void testConstructorInitialization() {
    CustomStyleSpan span = new CustomStyleSpan(2, 7, "sans-serif");

    assertEquals("Style should be initialized", 2, span.getStyle());
  }

  @Test
  public void testEquals() {
    CustomStyleSpan span1 = new CustomStyleSpan(1, 0, "Arial");
    CustomStyleSpan span2 = new CustomStyleSpan(1, 0, "Helvetica");
    CustomStyleSpan span3 = new CustomStyleSpan(2, 7, "Arial");

    assertTrue("Spans with same style/weight should be equal", span1.equals(span2));
    assertFalse("Different style should not be equal", span1.equals(span3));
    assertFalse("Null comparison should return false", span1.equals(null));
    assertFalse("Different class should return false", span1.equals(new Object()));
  }

  @Test
  public void testHashCodeConsistency() {
    CustomStyleSpan span1 = new CustomStyleSpan(1, 0, "Arial");
    CustomStyleSpan span2 = new CustomStyleSpan(1, 0, "Helvetica");

    assertEquals(
        "Hash codes should match for same style/weight", span1.hashCode(), span2.hashCode());
  }

  @Test
  public void testUpdateMethodsCallApply() {
    CustomStyleSpan span = new CustomStyleSpan(2, 0, "monospace");

    // Test measure state update
    span.updateMeasureState(mTextPaint);
    verifyTextPaintUpdate(2, 400);

    // Test draw state update
    span.updateDrawState(mTextPaint);
    verifyTextPaintUpdate(2, 400);
  }

  private void verifyTextPaintUpdate(int expectedStyle, int expectedWeight) {
    assertEquals(mTextPaint.getTypeface().getStyle(), expectedStyle);
    assertEquals(mTextPaint.getTypeface().getWeight(), expectedWeight);
  }

  @Test
  public void testNullFontFamilyHandling() {
    CustomStyleSpan span = new CustomStyleSpan(0, 0, null);
    span.updateDrawState(mTextPaint);

    verifyTextPaintUpdate(0, 400);
  }

  @Test
  public void testGetStyleMethod() {
    CustomStyleSpan span = new CustomStyleSpan(3, 500, "Roboto");
    assertEquals("getStyle() should return initialized value", 3, span.getStyle());
  }
}
