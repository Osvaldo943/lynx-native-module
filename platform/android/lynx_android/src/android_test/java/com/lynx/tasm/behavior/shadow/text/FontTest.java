// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Typeface;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class FontTest {
  String mDefaultFontFamily;
  Typeface mDefaultTypeface;
  LynxContext mLynxContext;

  @Before
  public void setUp() {
    mLynxContext = TestingUtils.getLynxContext();
    mDefaultFontFamily = "test-font";
    mDefaultTypeface = Typeface.create(mDefaultFontFamily, 0);

    TypefaceCache.addLazyProvider((fontFamilyName, style) -> {
      if (fontFamilyName.equals(mDefaultFontFamily)) {
        return mDefaultTypeface;
      } else {
        return null;
      }
    });
  }

  @After
  public void tearDown() {
    mDefaultTypeface = null;
  }

  @Test
  public void testLazyProvider() {
    Typeface tp = TypefaceCache.getTypeface(mLynxContext, mDefaultFontFamily, 0);
    Assert.assertTrue("should get default font from layProvider", tp == mDefaultTypeface);

    // clear all cached font from lazy provider
    TypefaceCache.removePreRegisteredFontCache(null);

    Typeface tp1 = Typeface.create("serif", 0);
    mDefaultTypeface = tp1;

    tp = TypefaceCache.getTypeface(mLynxContext, mDefaultFontFamily, 0);
    Assert.assertTrue("should get updated font from layProvider", tp == tp1);

    // clear specific font-family cache
    TypefaceCache.removePreRegisteredFontCache(mDefaultFontFamily);
    Typeface tp2 = Typeface.create("monospace", 0);
    mDefaultTypeface = tp2;

    tp = TypefaceCache.getTypeface(mLynxContext, mDefaultFontFamily, 0);
    Assert.assertTrue("should get updated font after removing the specific font-family", tp == tp2);
  }
}
