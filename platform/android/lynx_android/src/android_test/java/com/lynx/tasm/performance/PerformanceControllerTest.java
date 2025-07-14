// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static org.mockito.Mockito.*;

import com.lynx.tasm.TimingHandler;
import org.junit.Before;
import org.junit.Test;

public class PerformanceControllerTest {
  private PerformanceController performanceController;

  @Before
  public void setUp() {
    performanceController = spy(new PerformanceController());
  }

  @Test
  public void testSetExtraTiming() throws Exception {
    TimingHandler.ExtraTimingInfo extraTimingInfo = new TimingHandler.ExtraTimingInfo();
    extraTimingInfo.mOpenTime = 1000;
    extraTimingInfo.mContainerInitStart = 2000;
    extraTimingInfo.mContainerInitEnd = 3000;
    extraTimingInfo.mPrepareTemplateStart = 4000;
    extraTimingInfo.mPrepareTemplateEnd = 5000;

    performanceController.setExtraTiming(extraTimingInfo);
  }
}
