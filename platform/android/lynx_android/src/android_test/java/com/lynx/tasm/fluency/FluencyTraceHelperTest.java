// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.fluency;

import static org.mockito.Mockito.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import junit.framework.TestCase;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class FluencyTraceHelperTest extends TestCase {
  private FluencyTraceHelper mFluencyTraceHelper;
  private FluencyTracerImpl mMockTracer;

  @Before
  public void setUp() throws Exception {
    super.setUp();
    LynxContext lynxContext = TestingUtils.getLynxContext();
    mMockTracer = mock(FluencyTracerImpl.class);
    mFluencyTraceHelper = new FluencyTraceHelper(lynxContext);
    Field tracerField = mFluencyTraceHelper.getClass().getDeclaredField("mTracer");
    tracerField.setAccessible(true);
    tracerField.set(mFluencyTraceHelper, mMockTracer);
  }

  @Test
  public void testStart() {
    int sign = 123;
    String scene = "testScene";
    String tag = "testTag";
    mFluencyTraceHelper.setPageConfigProbability(1.0);
    mFluencyTraceHelper.start(sign, scene, tag);

    verify(mMockTracer)
        .start(eq(sign),
            argThat(config
                -> config.getScene().equals(scene) && config.getTag().equals(tag)
                    && config.getPageConfigProbability() == 1.0));
  }

  @Test
  public void testStop() {
    int sign = 123;
    mFluencyTraceHelper.setPageConfigProbability(1.0);
    mFluencyTraceHelper.stop(sign);

    verify(mMockTracer).stop(sign);
  }

  @Test
  public void testShouldSendAllScrollEvent() throws Exception {
    // Default behavior
    assertFalse(mFluencyTraceHelper.shouldSendAllScrollEvent());

    // Settings ON
    Field forceEnableField = FluencySample.class.getDeclaredField("sForceEnable");
    forceEnableField.setAccessible(true);
    forceEnableField.setBoolean(FluencySample.class, true);
    assertTrue(mFluencyTraceHelper.shouldSendAllScrollEvent());

    // Force OFF
    mFluencyTraceHelper.setPageConfigProbability(0.0);
    assertFalse(mFluencyTraceHelper.shouldSendAllScrollEvent());

    // Force ON
    mFluencyTraceHelper.setPageConfigProbability(1.0);
    assertTrue(mFluencyTraceHelper.shouldSendAllScrollEvent());
  }
}
