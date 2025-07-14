// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.util.AttributeSet;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxFrameView extends UIBodyView {
  private LynxTemplateRender mRender;

  public LynxFrameView(Context context) {
    super(context);
    init(context);
  }

  public LynxFrameView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init(context);
  }

  private void init(Context context) {
    // TODO(zhoupeng.z): get builder info from LynxView
    LynxViewBuilder builder = new LynxViewBuilder();
    mLynxUIRender = builder.createLynxUIRenderer();
    mRender = new LynxTemplateRender(context, this, builder);
  }

  void loadBundle(TemplateBundle bundle) {
    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setTemplateBundle(bundle);
    mRender.loadTemplate(builder.build());
  }

  public void updateViewport(int widthMeasureSpec, int heightMeasureSpec) {
    mRender.updateViewport(widthMeasureSpec, heightMeasureSpec);
  }

  @Override
  public void runOnTasmThread(Runnable runnable) {
    mRender.runOnTasmThread(runnable);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    mRender.onMeasure(widthMeasureSpec, heightMeasureSpec);
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    mRender.onLayout(changed, left, top, right, bottom);
  }

  void destroy() {
    mRender.destroy();
  }
}
