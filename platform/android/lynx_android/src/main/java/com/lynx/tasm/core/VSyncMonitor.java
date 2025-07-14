// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.Choreographer;
import android.view.Display;
import android.view.WindowManager;
import androidx.annotation.RequiresApi;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public class VSyncMonitor {
  public final static long DEFAULT_FRAME_TIME_NS = 1000000000 / 60;
  private static WeakReference<WindowManager> mWindowManager;

  private static DisplayManager mDisplayManager;
  private static Choreographer sUIThreadChoreographer = null;

  private static boolean mUseDisplayManager = false;

  private static DisplayManager.DisplayListener mDisplayListener = null;

  // FIXME(linxs): shall we need to make it volatile? Actually, we will make the VSyncMonitor to be
  // called on UI thread soon
  private static long mFrameRefreshTimeNS = -1;

  @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
  static class DisplayListener implements DisplayManager.DisplayListener {
    @Override
    public void onDisplayAdded(int displayId) {}

    @Override
    public void onDisplayRemoved(int displayId) {}

    @Override
    public void onDisplayChanged(int displayId) {
      if (displayId == Display.DEFAULT_DISPLAY && mDisplayManager != null) {
        updateFreshRate();
      }
    }
  }

  private static void updateFreshRate() {
    try {
      final Display primaryDisplay;
      if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1) {
        primaryDisplay = mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY);
        float fps = primaryDisplay.getRefreshRate();
        mFrameRefreshTimeNS = (long) (1000000000 / fps);
      } else {
        mFrameRefreshTimeNS = DEFAULT_FRAME_TIME_NS;
      }
    } catch (RuntimeException e) {
      LLog.e("VSyncMonitor",
          "onDisplayChanged failed: " + CallStackUtil.getStackTraceStringTrimmed(e));
    }
  }

  public static void setCurrentWindowManager(WindowManager vm) {
    mWindowManager = new WeakReference<>(vm);
    mUseDisplayManager = false;
  }

  @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
  public static void setCurrentDisplayManager(DisplayManager dm) {
    if (dm != null) {
      mDisplayManager = dm;
      updateFreshRate();
      if (mDisplayListener == null) {
        mDisplayListener = new DisplayListener();
      }
      dm.registerDisplayListener(mDisplayListener, new Handler(Looper.getMainLooper()));
      mUseDisplayManager = true;
    }
  }

  public static void initUIThreadChoreographer() {
    if (sUIThreadChoreographer != null) {
      return;
    }

    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        try {
          sUIThreadChoreographer = Choreographer.getInstance();
        } catch (RuntimeException e) {
          LLog.e("VSyncMonitor",
              "initUIThreadChoreographer failed: " + CallStackUtil.getStackTraceStringTrimmed(e));
        }
      }
    });
  }

  @CalledByNative
  public static void request(final long nativePtr) {
    Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        VSyncMonitor.doFrame(nativePtr, frameTimeNanos);
      }
    });
  }

  @CalledByNative
  public static void requestOnUIThread(final long nativePtr) {
    if (sUIThreadChoreographer == null) {
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          initUIThreadChoreographer();
          requestOnUIThread(nativePtr);
        }
      });
    } else {
      sUIThreadChoreographer.postFrameCallback(new Choreographer.FrameCallback() {
        @Override
        public void doFrame(long frameTimeNanos) {
          VSyncMonitor.doFrame(nativePtr, frameTimeNanos);
        }
      });
    }
  }

  private static long getRefreshRate() {
    if (!mUseDisplayManager) {
      try {
        mFrameRefreshTimeNS = DEFAULT_FRAME_TIME_NS;
        WindowManager wm = mWindowManager.get();
        if (wm != null) {
          mFrameRefreshTimeNS = (long) (1000000000.0 / wm.getDefaultDisplay().getRefreshRate());
        }
      } catch (RuntimeException e) {
        // These code contains an inter-process communication, which may throw DeadSystemException.
        // And inside DisplayManagerGlobal, the DeadSystemException is wrapped into a
        // RuntimeException and rethrown, so we attempt to catch RuntimeException here.
        LLog.e("VSyncMonitor", "getRefreshRate failed: " + e.getMessage());
      }
    }

    return mFrameRefreshTimeNS;
  }

  private static void doFrame(long nativePtr, long frameTimeNanos) {
    nativeOnVSync(nativePtr, frameTimeNanos, frameTimeNanos + getRefreshRate());
  }

  private static native void nativeOnVSync(
      long nativePtr, long frameStartTimeNS, long frameEndTimeNS);
}
