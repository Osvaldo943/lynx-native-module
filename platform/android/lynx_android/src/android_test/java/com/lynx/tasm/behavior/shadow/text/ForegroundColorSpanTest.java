// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.graphics.Color;
import android.graphics.Paint;
import android.text.TextPaint;
import org.junit.Test;

public class ForegroundColorSpanTest {
  @Test
  public void testConstructor() {
    int expectedColor = Color.RED;
    ForegroundColorSpan span = new ForegroundColorSpan(expectedColor);
    assertEquals(expectedColor, span.getForegroundColor());
  }

  @Test
  public void testEquals() {
    ForegroundColorSpan span1 = new ForegroundColorSpan(0xFF00FF);
    ForegroundColorSpan span2 = new ForegroundColorSpan(0xFF00FF);
    ForegroundColorSpan span3 = new ForegroundColorSpan(0x00FF00);

    assertTrue(span1.equals(span2));
    assertFalse(span1.equals(span3));
    assertFalse(span1.equals(null));
    assertFalse(span1.equals(new Object()));
  }

  @Test
  public void testHashCodeConsistency() {
    int color = 0x12345678;
    ForegroundColorSpan span = new ForegroundColorSpan(color);
    assertEquals(31 + color, span.hashCode());
  }

  @Test
  public void testUpdateDrawState() {
    ForegroundColorSpan span = new ForegroundColorSpan(Color.CYAN);
    span.setStrokeColor(Color.MAGENTA);
    span.setStrokeWidth(3.0f);
    span.setDrawStroke(true);

    TextPaint tp = new TextPaint();
    tp.setStyle(Paint.Style.FILL);
    span.updateDrawState(tp);

    assertEquals("Stroke width is wrong", 3.0f, tp.getStrokeWidth(), 0.001f);
    assertEquals("Paint style is wrong", Paint.Style.STROKE, tp.getStyle());
  }
}
