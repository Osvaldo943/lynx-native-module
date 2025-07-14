// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

public enum LogBoxLogLevel {
  Info("info"),
  Warn("warn"),
  Error("error");

  public final String value;

  LogBoxLogLevel(String value) {
    this.value = value;
  }
}
