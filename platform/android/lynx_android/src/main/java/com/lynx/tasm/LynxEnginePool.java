// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

class LynxEnginePool {
  private static final String TAG = "LynxEngineWrapperPool";

  private Map<TemplateBundle, Deque<LynxEngine>> mCache = new HashMap<>();

  private static class Holder {
    private static final LynxEnginePool INSTANCE = new LynxEnginePool();
  }

  static LynxEnginePool getInstance() {
    return Holder.INSTANCE;
  }

  void registerReuseEngineWrapper(LynxEngine engineWrapper) {
    Deque<LynxEngine> engineQueue = getEngineQueue(engineWrapper.getTemplateBundle());
    synchronized (this) {
      engineQueue.offer(engineWrapper);
    }
    engineWrapper.setQueueRefFromPool(engineQueue);
    LLog.i(
        TAG, "registerReuseEngineWrapper EngineQueue Cache: " + engineQueue + "," + engineWrapper);
  }

  @Nullable
  LynxEngine pollEngineFromPool(TemplateBundle templateBundle) {
    Deque<LynxEngine> engineQueue = getEngineQueue(templateBundle);
    LLog.i(TAG, "pollEngine EngineQueue Cache: " + engineQueue + ", bundle:" + templateBundle);
    LynxEngine lynxEngine = null;
    synchronized (this) {
      while (!engineQueue.isEmpty()) {
        lynxEngine = engineQueue.poll();
        if (lynxEngine != null && lynxEngine.canReused()) {
          break;
        }
      }
    }
    if (lynxEngine != null && lynxEngine.canReused()) {
      lynxEngine.detachEngine();
      return lynxEngine;
    }
    return null;
  }

  @NonNull
  private synchronized Deque<LynxEngine> getEngineQueue(TemplateBundle templateBundle) {
    Deque<LynxEngine> engineQueue = mCache.get(templateBundle);
    if (engineQueue == null) {
      engineQueue = new LinkedList<>();
      mCache.put(templateBundle, engineQueue);
      // FIXME(huangweiwu) : Attach as needed, no need for active
      // release.(templateBundle.setReleaseCallback if need)
    }
    return engineQueue;
  }
}
