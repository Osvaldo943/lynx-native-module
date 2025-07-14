// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.content.Context;
import com.lynx.basedevtool.logbox.ILogBoxResourceProvider;
import com.lynx.basedevtool.logbox.LogBoxLogLevel;
import com.lynx.basedevtool.logbox.LogBoxProxy;
import com.lynx.devtoolwrapper.ILynxLogBox;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxLogBoxWrapper implements ILynxLogBox, ILogBoxResourceProvider {
  private static final String TAG = "LynxLogBoxWrapper";

  private LogBoxProxy mLogBoxProxy;
  private WeakReference<LynxDevtool> mDevtool;

  public LynxLogBoxWrapper(LynxDevtool devtool) {
    mDevtool = new WeakReference<>(devtool);
    Context context = null;
    if (devtool != null) {
      context = devtool.getLynxContext();
    }
    mLogBoxProxy = new LogBoxProxy(context, "lynx", this);
    mLogBoxProxy.registerErrorParser("logbox/lynx-error-parser.js");
  }

  @Override
  public void showLogMessage(final LynxError error) {
    String message = error.getMsg();
    LogBoxLogLevel level =
        error.getLevel().equals(LynxError.LEVEL_WARN) ? LogBoxLogLevel.Warn : LogBoxLogLevel.Error;
    sendErrorEventToPerf(message, level);
    mLogBoxProxy.showLogMessage(message, level);
  }

  @Override
  public void onLoadTemplate() {
    mLogBoxProxy.reset();
  }

  @Override
  public void attachContext(Context context) {
    mLogBoxProxy.attachContext(context);
  }

  @Override
  public String getEntryUrlForLogSrc() {
    LynxDevtool devtool = mDevtool.get();
    return devtool == null ? "" : devtool.getTemplateUrl();
  }

  @Override
  public Map<String, Object> getLogSources() {
    LynxDevtool devtool = mDevtool.get();
    return devtool == null ? null : devtool.getAllJsSource();
  }

  @Override
  public String getLogSourceWithFileName(String fileName) {
    LynxDevtool devtool = mDevtool.get();
    if (devtool == null) {
      return "";
    }
    if (fileName.endsWith("main-thread.js")) {
      return devtool.getDebugInfoUrl(fileName);
    } else {
      Map<String, Object> logSources = getLogSources();
      String value = "";
      int matchLength = 0;
      for (Map.Entry<String, Object> entry : logSources.entrySet()) {
        String key = entry.getKey();
        if (fileName.endsWith(key) && key.length() > matchLength) {
          matchLength = key.length();
          value = (String) entry.getValue();
        }
      }
      return value;
    }
  }

  protected void sendErrorEventToPerf(final String message, final LogBoxLogLevel level) {
    if (level == LogBoxLogLevel.Info) {
      return;
    }
    LynxDevtool devtool = mDevtool.get();
    if (devtool == null) {
      return;
    }
    try {
      JSONObject eventData = new JSONObject();
      eventData.put("error", message);
      devtool.onPerfMetricsEvent("lynx_error_event", eventData);
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
  }
}
