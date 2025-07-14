// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import static org.junit.Assert.*;

import android.content.Context;
import org.junit.Before;
import org.junit.Test;

public class LogBoxEnvTest {
  private Boolean mFlag;

  @Before
  public void setUp() {
    mFlag = false;
  }

  @Test
  public void registerErrorParserLoader() {
    LogBoxEnv.ILogBoxErrorParserLoader loader = new LogBoxEnv.ILogBoxErrorParserLoader() {
      @Override
      public void loadErrorParser(Context context, DevToolFileLoadCallback callback) {
        mFlag = true;
        callback.onSuccess("test data");
        callback.onFailure("test error");
      }
    };
    LogBoxEnv.inst().registerErrorParserLoader("register-test", loader);
    LogBoxEnv.inst().loadErrorParser(null, "register-test", new DevToolFileLoadCallback() {
      @Override
      public void onSuccess(String data) {
        assertEquals(data, "test data");
      }

      @Override
      public void onFailure(String reason) {
        assertEquals(reason, "test error");
      }
    });
    assertTrue(mFlag);
  }
}
