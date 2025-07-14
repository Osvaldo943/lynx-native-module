// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import android.content.Context;
import android.content.Intent;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxView;
import java.util.ArrayList;
import java.util.Arrays;

public class LynxRecorderView extends RelativeLayout {
  public static final String LYNX_VIEW_WIDTH = "LynxViewWidth";
  public static final String LYNX_VIEW_HEIGHT = "LynxViewHeight";
  private int[] mPoint;

  private LynxRecorderActionManager mActionManager;
  private LynxView mLynxView;

  public LynxRecorderView(Context context) {
    super(context);
  }

  public void reload() {
    mActionManager.load();
    if (mLynxView != null) {
      mLynxView.onEnterForeground();
    }
  }

  public void destroy() {
    if (mActionManager != null) {
      mActionManager.destroy();
    }
  }

  public void updateScreenMetrics(int widthPixels, int heightPixels) {
    if (mLynxView != null) {
      mLynxView.updateScreenMetrics(widthPixels, heightPixels);
    }
  }

  public void loadPageWithPoint(String url, int[] point, Intent intent) {
    mPoint = Arrays.copyOf(point, point.length);
    mActionManager = new LynxRecorderActionManager(intent, this.getContext(), this, null);
    mActionManager.registerCallback(new LynxRecorderActionCallback() {
      @Override
      public void onLynxViewDidBuild(@NonNull LynxView kitView, @NonNull Intent intent,
          @NonNull Context context, @NonNull ViewGroup viewGroup) {
        mLynxView = kitView;
        attachToView(viewGroup, intent.getIntExtra(LYNX_VIEW_WIDTH, MATCH_PARENT),
            intent.getIntExtra(LYNX_VIEW_HEIGHT, MATCH_PARENT));
      }
    });
    ArrayList<LynxRecorderActionCallback> externalCallbacks =
        LynxRecorderPageManager.getInstance().getCallbacks();
    for (LynxRecorderActionCallback callback : externalCallbacks) {
      mActionManager.registerCallback(callback);
    }
    mActionManager.startWithUrl(url);
  }

  private void attachToView(ViewGroup view, int width, int height) {
    if (mLynxView != null) {
      view.addView(mLynxView, new LinearLayout.LayoutParams(width, height));
      RelativeLayout.LayoutParams layoutParams =
          (RelativeLayout.LayoutParams) this.getLayoutParams();
      layoutParams.width = WRAP_CONTENT;
      layoutParams.height = WRAP_CONTENT;
      layoutParams.leftMargin = mPoint[0];
      layoutParams.topMargin = mPoint[1];
      this.setLayoutParams(layoutParams);
    }
  }
}
