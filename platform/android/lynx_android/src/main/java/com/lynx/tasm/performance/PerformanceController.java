// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.memory.IMemoryMonitor;
import com.lynx.tasm.performance.memory.IMemoryRecordBuilder;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.performance.timing.ITimingCollector;
import com.lynx.tasm.performance.timing.TimingUtil;
import com.lynx.tasm.service.ILynxEventReporterService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.ref.WeakReference;
import java.util.HashMap;

/**
 * @brief Manages performance data collection and observation. This class acts as a central point
 * for performance-related events, conforming to timing collection, memory monitoring, and
 * performance observation protocols.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class PerformanceController implements IMemoryMonitor, ITimingCollector {
  private static volatile boolean sIsNativeLibraryLoaded = false;
  private static volatile LynxBooleanOption sIsMemoryMonitorEnabled = LynxBooleanOption.UNSET;
  private volatile long mNativePerformanceActorPtr = 0;
  private WeakReference<IPerformanceObserver> mObserver;
  private WeakReference<ILynxEventReporterService> mEventReporterService;
  private boolean mEnableController = true;

  /**
   * Checks if memory monitoring is enabled.
   * Modules can call this before collecting data to avoid unnecessary collection.
   */
  public static boolean isMemoryMonitorEnabled() {
    if (isNativeLibraryLoaded()) {
      LynxBooleanOption op = sIsMemoryMonitorEnabled;
      if (op == LynxBooleanOption.FALSE) {
        return false;
      } else if (op == LynxBooleanOption.UNSET) {
        boolean ret = nativeIsMemoryMonitorEnabled();
        sIsMemoryMonitorEnabled = ret ? LynxBooleanOption.TRUE : LynxBooleanOption.FALSE;
        return ret;
      }
      return true;
    }
    return false;
  }

  public void setPerformanceObserver(IPerformanceObserver observer) {
    mObserver = new WeakReference<>(observer);
  }

  public boolean isEnableController() {
    return mEnableController;
  }

  /**
   * Call this interface to disable performance monitoring only in Embedded Mode.
   */
  public void setEnableController(boolean enableController) {
    mEnableController = enableController;
  }

  @Override
  public void allocateMemory(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeAllocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void deallocateMemory(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeDeallocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void updateMemoryUsage(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeUpdateMemoryUsage(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void setMsTiming(String key, long msTimestamp, String pipelineID) {
    if (!mEnableController) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, msTimestamp * 1000, pipelineID);
    });
  }

  @Override
  public void markTiming(final String key, final String pipelineID) {
    if (!mEnableController) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, usTimestamp, pipelineID);
    });
  }

  @Override
  public void markPaintEndTimingIfNeeded() {
    if (!mEnableController) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetPaintEndTimingIfNeeded(mNativePerformanceActorPtr, usTimestamp);
    });
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (!mEnableController) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      if (extraTiming.mOpenTime > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.OPEN_TIME,
            extraTiming.mOpenTime * 1000, null);
      }
      if (extraTiming.mContainerInitStart > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.CONTAINER_INIT_START,
            extraTiming.mContainerInitStart * 1000, null);
      }
      if (extraTiming.mContainerInitEnd > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.CONTAINER_INIT_END,
            extraTiming.mContainerInitEnd * 1000, null);
      }
      if (extraTiming.mPrepareTemplateStart > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.PREPARE_TEMPLATE_START,
            extraTiming.mPrepareTemplateStart * 1000, null);
      }
      if (extraTiming.mPrepareTemplateEnd > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.PREPARE_TEMPLATE_END,
            extraTiming.mPrepareTemplateEnd * 1000, null);
      }
    });
  }

  @CalledByNative
  protected void setNativePtr(long nativePtr) {
    if (!mEnableController) {
      return;
    }
    mNativePerformanceActorPtr = nativePtr;
  }

  @CalledByNative
  protected void onPerformanceEvent(ReadableMap entryMap) {
    if (!mEnableController) {
      return;
    }
    IPerformanceObserver observer = mObserver.get();
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (observer != null) {
      observer.onPerformanceEvent(entry);
    }

    if (mEventReporterService == null) {
      ILynxEventReporterService reporter =
          LynxServiceCenter.inst().getService(ILynxEventReporterService.class);
      if (reporter != null) {
        mEventReporterService = new WeakReference<>(reporter);
      }
    }
    ILynxEventReporterService reporter = mEventReporterService.get();
    if (reporter != null) {
      int instanceId = entryMap.getInt("instanceId");
      if (instanceId > 0) {
        HashMap<String, Object> newEntryMap = LynxEventReporter.getGenericInfo(instanceId);
        newEntryMap.putAll(entryMap.asHashMap());
        PerformanceEntry newEntry =
            PerformanceEntryConverter.makePerformanceEntry(JavaOnlyMap.from(newEntryMap));
        reporter.onPerformanceEvent(newEntry);
      }
    }
  }

  @AnyThread
  private void runOnReportThread(Runnable runnable) {
    if (!mEnableController) {
      return;
    }
    LynxEventReporter.runOnReportThread(runnable);
  }

  private static boolean isNativeLibraryLoaded() {
    if (!sIsNativeLibraryLoaded) {
      sIsNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    }
    return sIsNativeLibraryLoaded;
  }

  // Native API
  private native void nativeAllocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeDeallocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeUpdateMemoryUsage(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeSetTiming(
      long nativePtr, String key, long usTimestamp, String pipelineID);
  private native void nativeSetPaintEndTimingIfNeeded(long nativePtr, long usTimestamp);
  private static native boolean nativeIsMemoryMonitorEnabled();
}
