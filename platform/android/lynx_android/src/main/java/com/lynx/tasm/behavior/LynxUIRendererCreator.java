// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import com.lynx.tasm.IUIRendererCreator;

public class LynxUIRendererCreator implements IUIRendererCreator {
  @Override
  public ILynxUIRenderer createLynxUIRender() {
    return new LynxUIRenderer();
  }
}
