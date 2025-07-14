// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;
import android.view.View;
import com.lynx.tasm.behavior.ui.ViewInfo;

// IProcessViewInfoHook is a crucial class for enabling the reuse of the Lynx Engine. In the future,
// LynxUI will inherit IProcessViewInfoHook, and ViewInfo will hold an instance of
// IProcessViewInfoHook. At critical stages, ViewInfo will invoke the interface methods of
// IProcessViewInfoHook to record rendering information.
interface IProcessViewInfoHook {
  void processViewInfo();
  void dispatchProcessViewInfo();
  void processChildViewInfo(IProcessViewInfoHook child);
  void beforeProcessViewInfo(ViewInfo info);
  void beforeDispatchProcessViewInfo(ViewInfo info);
  void beforeProcessChildViewInfo(ViewInfo info, View child, long drawingTime);
  void afterProcessChildViewInfo(ViewInfo info, View child, long drawingTime);
  void afterDispatchProcessViewInfo(ViewInfo info);
  void afterProcessViewInfo(ViewInfo info);
  void processLayoutChildren();
  void processMeasureChildren();
}
