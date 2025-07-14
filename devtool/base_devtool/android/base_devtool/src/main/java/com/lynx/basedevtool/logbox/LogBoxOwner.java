// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import java.util.WeakHashMap;

public class LogBoxOwner {
  private static final String TAG = "LogBoxOwner";
  private WeakHashMap<Context, LogBoxManager> mLogBoxManagers;

  public static LogBoxOwner getInstance() {
    return LogBoxOwner.SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final LogBoxOwner INSTANCE = new LogBoxOwner();
  }

  private LogBoxOwner() {
    mLogBoxManagers = new WeakHashMap<>();
  }

  public void dispatch(
      String msg, LogBoxLogLevel level, Context activity, LogBoxProxy logBoxProxy) {
    LogBoxManager manager = findManagerByActivity(activity);
    if (manager != null) {
      manager.onNewLog(msg, level, logBoxProxy);
    }
  }

  public void onProxyReset(Context activity, LogBoxProxy logBoxProxy) {
    LogBoxManager manager = findManagerByActivityIfExist(activity);
    // if the template is loaded for the first time, manager must be null
    if (manager != null) {
      manager.onProxyReset(logBoxProxy);
    }
  }

  private LogBoxManager findManagerByActivity(Context activity) {
    if (!(activity instanceof Activity)) {
      Log.e(TAG, "param activity is null or not a Activity");
      return null;
    }
    if (((Activity) activity).isFinishing()) {
      Log.i(TAG, "activity is finishing");
      return null;
    }
    LogBoxManager manager = mLogBoxManagers.get(activity);
    if (manager == null) {
      Log.i(TAG, "new activity");
      manager = new LogBoxManager(activity);
      mLogBoxManagers.put(activity, manager);
    }
    return manager;
  }

  protected LogBoxManager findManagerByActivityIfExist(Context activity) {
    if (activity == null) {
      Log.e(TAG, "activity is null");
      return null;
    }
    return mLogBoxManagers.get(activity);
  }
}
