// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fluency;

import androidx.annotation.UiThread;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.behavior.LynxContext;
import java.security.SecureRandom;

public class FluencyTraceHelper {
  private FluencyTracerImpl mTracer;

  private final SecureRandom mSecureRandom = new SecureRandom();
  public static final double UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY = -1;
  private double mPageConfigProbability = UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY;
  public enum ForceStatus { FORCED_ON, FORCED_OFF, NON_FORCED }
  private ForceStatus mStatus = ForceStatus.NON_FORCED;
  private boolean mProbabilityDetermined = false;
  private LynxBooleanOption mEnabled = LynxBooleanOption.UNSET;

  // @Deprecated: will be removed in 3.0
  @Deprecated private String mScene = "";
  @Deprecated private String mTag = "";

  public FluencyTraceHelper(LynxContext context) {
    setPageConfigProbability(context.getEnableLynxScrollFluency());
    mTracer = new FluencyTracerImpl(context);
  }

  // @Deprecated: will be removed in 3.0
  @Deprecated
  public FluencyTraceHelper(LynxContext context, String scene, String tag) {
    // enableLynxScrollFluency is undefined, just use settings.
    if (!FluencySample.isEnable()) {
      return;
    }

    if (context == null) {
      return;
    }
    mScene = scene;
    mTag = tag;
    mTracer = new FluencyTracerImpl(context);
  }

  /**
   * Configure `fluencyPageConfigProbability` in this context based on the specified probability.
   * This function generates a new random double. If the random value is smaller than the
   * probability, it updates `fluencyPageConfigProbability`. Otherwise, it sets the value
   * to 0.
   *
   * @param probability Double number. Used to indicate the probability of enabling FluencyMetrics
   *                    in this context configured by the front end.
   */
  public void setPageConfigProbability(double probability) {
    mPageConfigProbability = probability;
    mProbabilityDetermined = false; // re-test only when page config probability is set again.
    updateStatus();
  }

  /**
   * Update the fluency tracing enabled status by considering two options:
   * 1. `enabledBySampling`: Passed from LynxView.setFluencyTracerEnabled().
   * 2. `pageConfigProbability`: Passed from the FE PageConfig.SetEnableScrollFluencyMonitor().
   * This function generates a new random double. If the random value is smaller than the
   * probability, it updates `fluencyPageConfigProbability`.
   * Otherwise, it tests whether the `enabledBySampling` is set.
   *  If it's set to true, it sets the value to `ForceStatus.FORCED_ON`.
   *  If it's set to false, it sets the value to `ForceStatus.FORCED_OFF`.
   * Otherwise, the status remains unchanged.
   *
   * @param samplingEnabled LynxBooleanOption. Used to indicate the sampling
   *                        decision set on LynxView.
   */
  public void setEnabledBySampling(LynxBooleanOption enabledBySampling) {
    if (mEnabled == enabledBySampling) {
      return;
    }
    mEnabled = enabledBySampling;
    updateStatus();
  }

  private void updateStatus() {
    if (mPageConfigProbability >= 0) {
      if (mProbabilityDetermined && mStatus != ForceStatus.NON_FORCED) {
        return;
      }

      // roll the dice only when page config probability is set again and the status is not forced
      // on.
      mProbabilityDetermined = true;
      if (mSecureRandom.nextDouble() <= mPageConfigProbability) {
        mStatus = ForceStatus.FORCED_ON;
      } else {
        mStatus = ForceStatus.FORCED_OFF;
      }
    } else if (mEnabled != LynxBooleanOption.UNSET) {
      mStatus = mEnabled == LynxBooleanOption.TRUE ? ForceStatus.FORCED_ON : ForceStatus.FORCED_OFF;
    }
  }

  public boolean shouldSendAllScrollEvent() {
    if (mStatus == ForceStatus.NON_FORCED) {
      // enableLynxScrollFluency is undefined, just use settings.
      return FluencySample.isEnable();
    } else
      return mStatus == ForceStatus.FORCED_ON;
  }

  // @Deprecated: will be removed in 3.0
  @Deprecated
  public void start() {
    if (mTracer == null) {
      return;
    }
    FluencyTracerImpl.FluencyTracerConfig config = new FluencyTracerImpl.FluencyTracerConfig();
    config.setScene(mScene);
    config.setTag(mTag);
    config.setPageConfigProbability(mPageConfigProbability);
    // In the old implementation, FluencyTraceHelper corresponds to FluencyTracerImpl one by one,
    // and we do not need a sign to distinguish different UI. Therefore, we can directly pass in the
    // default value of 0.
    mTracer.start(0, config);
  }

  // @Deprecated: will be removed in 3.0
  @Deprecated
  public void stop() {
    if (mTracer == null) {
      return;
    }
    // In the old implementation, FluencyTraceHelper corresponds to FluencyTracerImpl one by one,
    // and we do not need a sign to distinguish different UI. Therefore, we can directly pass in the
    // default value of 0.
    mTracer.stop(0);
  }

  @UiThread
  public void start(int sign, String scene, String tag) {
    if (mTracer == null || !shouldSendAllScrollEvent()) {
      return;
    }
    FluencyTracerImpl.FluencyTracerConfig config = new FluencyTracerImpl.FluencyTracerConfig();
    config.setScene(scene);
    config.setTag(tag);
    config.setPageConfigProbability(mPageConfigProbability);
    config.setEnabledBySampling(mEnabled);
    mTracer.start(sign, config);
  }

  @UiThread
  public void stop(int sign) {
    if (mTracer == null || !shouldSendAllScrollEvent()) {
      return;
    }
    mTracer.stop(sign);
  }
}
