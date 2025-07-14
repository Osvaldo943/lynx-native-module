// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.explorer.shell;

import com.lynx.devtool.recorder.LynxRecorderActionCallback;
import com.lynx.devtool.recorder.LynxRecorderActionManager;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxGroup.LynxGroupBuilder;
import com.lynx.tasm.LynxViewBuilder;

public class LynxRecorderDefaultActionCallback implements LynxRecorderActionCallback {
  @Override
  public void onLynxViewWillBuild(LynxRecorderActionManager manager, LynxViewBuilder builder) {
    String[] paths = manager.getTestBenchPreloadScripts();

    LynxGroup lynxGroup = manager.getLynxGroup();
    if (lynxGroup == null) {
      lynxGroup =
          new LynxGroupBuilder().setGroupName("lynx_recorder").setPreloadJSPaths(paths).build();
      manager.setLynxGroup(lynxGroup);
    }
    builder.setLynxGroup((LynxGroup) lynxGroup);
  }
}
