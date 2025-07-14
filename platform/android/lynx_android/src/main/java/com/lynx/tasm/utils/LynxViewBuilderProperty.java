// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

public enum LynxViewBuilderProperty {
  /**
   * @param key platform_config, value: json string
   * Set the platform_config for LynxViewBuilder.LynxViewConfig.
   */
  PLATFORM_CONFIG("platform_config");

  private final String key;
  LynxViewBuilderProperty(String key) {
    this.key = key;
  }

  public String getKey() {
    return key;
  }
}
