// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.graphics.Rect;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxUI;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class UIFrame extends LynxUI<LynxFrameView> {
  public UIFrame(LynxContext context) {
    super(context);
  }

  @Override
  protected LynxFrameView createView(Context context) {
    return new LynxFrameView(mContext);
  }

  @Override
  public void updateExtraData(Object data) {
    if (data instanceof TemplateBundle) {
      LynxFrameView view = getView();
      if (view != null) {
        view.loadBundle((TemplateBundle) data);
      }
    }
  }

  @Override
  public void onNodeRemoved() {
    super.onNodeRemoved();
    LynxFrameView view = getView();
    if (view != null) {
      view.destroy();
    }
  }

  @Override
  public void updateLayout(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    super.updateLayout(left, top, width, height, paddingLeft, paddingTop, paddingRight,
        paddingBottom, marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth,
        borderTopWidth, borderRightWidth, borderBottomWidth, bound);

    LynxFrameView view = getView();
    if (view != null) {
      view.updateViewport(width, height);
    }
  }
}
