// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import java.lang.ref.WeakReference;
import java.util.Deque;

/**
 * Do not make this class public until a proxy class is added to decouple it from the Pool.
 */
class LynxEngine {
  enum LynxEngineState {
    UN_LOADED,
    READY_BE_REUSED,
    ON_REUSING,
    FREEZED,
    DESTROYED,
  }

  private static final String TAG = "LynxEngineWrapper";

  @Nullable private LynxTemplateRender mLynxTemplateRender;

  private long mNativePtr;
  private TemplateBundle mTemplateBundle;
  private LynxEngineState mLynxEngineState = LynxEngineState.UN_LOADED;
  private WeakReference<Deque<LynxEngine>> mLynxEngineWrapperQueue;

  private ILynxUIRenderer mLynxUIRenderer;
  // LynxContext or PageConfig
  private PageConfig mPageConfig;

  public LynxEngine(@NonNull TemplateBundle templateBundle, LynxTemplateRender templateRender) {
    /**
     * 1. Check if there is an available EngineWrapper and retrieve it from the cache.
     *  1.1 If no EngineWrapper is available, create one and obtain the nativePtr of the
     * EngineWrapper.
     * 2. Create the Shell.
     *  2.1 Pass in the nativePtr of the EngineWrapper to complete the relationship binding between
     * the Shell and the Wrapper. 2.2 Create the PlatformContext and bind it to the Wrapper.
     * 3. Load the template card.
     *  3.1 Unbind and add to the EnginePool queue.
     * 4. When the card is destroyed, the Engine is not destroyed. It waits for the EnginePool to
     * request destruction.
     */
    // Note: At this time, the Engine object does not exist yet because it still depends on the
    // complete information of the Shell for creation.
    //       Therefore, the object information needs to be provided to the Wrapper for management
    //       after the Shell is created.
    mNativePtr = nativeCreate();
    mLynxTemplateRender = templateRender;
    mTemplateBundle = templateBundle;
  }

  public TemplateBundle getTemplateBundle() {
    return mTemplateBundle;
  }

  public long getNativePtr() {
    return mNativePtr;
  }

  public void attachCurrentTemplateRender(LynxTemplateRender templateRender) {
    mLynxTemplateRender = templateRender;
  }

  public void setQueueRefFromPool(Deque<LynxEngine> lynxEngineQueue) {
    this.mLynxEngineWrapperQueue = new WeakReference<>(lynxEngineQueue);
  }

  public ILynxUIRenderer getLynxUIRenderer() {
    return mLynxUIRenderer;
  }

  public void setLynxUIRenderer(ILynxUIRenderer lynxUIRenderer) {
    this.mLynxUIRenderer = lynxUIRenderer;
  }

  public PageConfig getPageConfig() {
    return mPageConfig;
  }

  public void setPageConfig(PageConfig pageConfig) {
    this.mPageConfig = pageConfig;
  }

  public void detachEngine() {
    if (mNativePtr != 0 && mLynxTemplateRender != null) {
      // FIXME(huangweiwu): remove templateRender? only setup the release callback here.
      if (mLynxTemplateRender.getLynxContext() != null
          && mLynxTemplateRender.getLynxContext().getUIBody() != null) {
        // TODO(songshourui): async detach
        mLynxTemplateRender.getLynxContext().getUIBody().detachUIBodyView();
      }
      mLynxTemplateRender.detachLynxEngineWrapper();
      mLynxTemplateRender = null;
    }
  }

  public LynxEngineState getLynxEngineState() {
    return mLynxEngineState;
  }

  public void setLynxEngineState(LynxEngineState LynxEngineState) {
    this.mLynxEngineState = LynxEngineState;
  }

  public boolean hasLoaded() {
    return mLynxEngineState != LynxEngineState.UN_LOADED;
  }

  public boolean canReused() {
    return mLynxEngineState == LynxEngineState.READY_BE_REUSED;
  }

  public boolean isRunOnCurrentTemplateRender(LynxTemplateRender templateRender) {
    if (mLynxTemplateRender == null) {
      return false;
    }
    return mLynxTemplateRender == templateRender;
  }

  public void registerLynxEngineReused() {
    mLynxEngineState = LynxEngineState.READY_BE_REUSED;
    LynxEnginePool.getInstance().registerReuseEngineWrapper(this);
  }

  public void destroy() {
    if (mLynxEngineWrapperQueue != null && mLynxEngineWrapperQueue.get() != null) {
      mLynxEngineWrapperQueue.get().remove(this);
    }
    if (mNativePtr != 0) {
      mLynxEngineState = LynxEngineState.DESTROYED;
      long nativePtr = mNativePtr;
      mNativePtr = 0;
      nativeDestroyEngine(nativePtr);
    }
  }

  private native long nativeCreate();

  private native long nativeDetachEngine(long nativePtr);

  private native void nativeDestroyEngine(long nativePtr);

  @Override
  public String toString() {
    return "LynxEngineWrapper{"
        + "mNativePtr=" + mNativePtr + ", mLynxEngineState=" + mLynxEngineState + '}';
  }
}
