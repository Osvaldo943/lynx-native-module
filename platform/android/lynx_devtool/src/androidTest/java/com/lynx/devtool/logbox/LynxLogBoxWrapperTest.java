// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import com.lynx.basedevtool.logbox.LogBoxLogLevel;
import com.lynx.devtoolwrapper.LynxDevtool;
import java.util.HashMap;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class LynxLogBoxWrapperTest {
  @Mock private LynxDevtool mockDevtool;

  private LynxLogBoxWrapper lynxLogBoxWrapper;

  @Before
  public void setUp() {
    lynxLogBoxWrapper = new LynxLogBoxWrapper(mockDevtool);
  }

  @Test
  public void testSendErrorEventToPerf_InfoLevel_ShouldNotSendEvent() {
    String message = "Test message";
    LogBoxLogLevel level = LogBoxLogLevel.Info;
    lynxLogBoxWrapper.sendErrorEventToPerf(message, level);
    verify(mockDevtool, never()).onPerfMetricsEvent(anyString(), any(JSONObject.class));
  }

  @Test
  public void testSendErrorEventToPerf_NullDevtool_ShouldNotSendEvent() {
    String message = "Test message";
    LogBoxLogLevel level = LogBoxLogLevel.Error;
    lynxLogBoxWrapper = new LynxLogBoxWrapper(null);
    lynxLogBoxWrapper.sendErrorEventToPerf(message, level);
    verify(mockDevtool, never()).onPerfMetricsEvent(anyString(), any(JSONObject.class));
  }

  @Test
  public void testSendErrorEventToPerf_ValidError_ShouldSendEvent() throws Exception {
    String message = "Test error message";
    LogBoxLogLevel level = LogBoxLogLevel.Error;
    lynxLogBoxWrapper.sendErrorEventToPerf(message, level);
    verify(mockDevtool).onPerfMetricsEvent(eq("lynx_error_event"), argThat(json -> {
      try {
        return json.getString("error").equals(message);
      } catch (Exception e) {
        return false;
      }
    }));
  }

  @Test
  public void testGetLogSourceWithFileName() {
    LynxLogBoxWrapper testWrapper = new LynxLogBoxWrapper(null);
    String result = testWrapper.getLogSourceWithFileName("");
    assertEquals(result, "");

    String mtsFileName = "file:///main-thread.js";
    String expectedUrl = "http://test-url/debug-info.json";
    when(mockDevtool.getDebugInfoUrl(anyString())).thenReturn(expectedUrl);
    result = lynxLogBoxWrapper.getLogSourceWithFileName(mtsFileName);
    assertEquals(expectedUrl, result);

    String fileName1 = "test.js";
    String fileName2 = "name/test.js";
    String expectedLogSource1 = "test";
    String expectedLogSource2 = "name/test";
    when(mockDevtool.getAllJsSource()).thenReturn(new HashMap<String, Object>() {
      {
        put(fileName1, expectedLogSource1);
        put(fileName2, expectedLogSource2);
      }
    });

    result = lynxLogBoxWrapper.getLogSourceWithFileName("file:///test.js");
    assertEquals(expectedLogSource1, result);
    result = lynxLogBoxWrapper.getLogSourceWithFileName("file://name/test.js");
    assertEquals(expectedLogSource2, result);
  }
}
