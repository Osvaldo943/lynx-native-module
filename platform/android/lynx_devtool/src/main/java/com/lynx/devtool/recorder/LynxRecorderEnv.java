// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

public class LynxRecorderEnv {
  private static LynxRecorderEnv instance;
  public String mUriKey = "uri";
  public String lynxRecorderUrlPrefix = "file://lynxrecorder?";

  public static synchronized LynxRecorderEnv getInstance() {
    if (instance == null) {
      instance = new LynxRecorderEnv();
    }
    return instance;
  }
}
