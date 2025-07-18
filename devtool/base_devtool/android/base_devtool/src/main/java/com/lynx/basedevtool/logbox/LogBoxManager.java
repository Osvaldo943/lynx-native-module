// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.app.Activity;
import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

public class LogBoxManager {
  private static final String TAG = "LogBoxManager";
  private static final String LIFE_CYCLE_LISTENER_FRAGMENT_TAG =
      "BaseDevToolLogBoxLifeCycleListener";
  private static final int MAX_BRIEF_MSG_SIZE = 50;

  private final WeakReference<Context> mContext;
  private Map<LogBoxLogLevel, LogProxyList> mLogProxyListMap;
  private LogBoxNotification mNotification;
  private LogBoxDialog mLogBox;

  public LogBoxManager(Context context) {
    mContext = new WeakReference<>(context);
    mLogProxyListMap = new HashMap<>();
    mLogProxyListMap.put(LogBoxLogLevel.Error, new LogProxyList(LogBoxLogLevel.Error));
    mLogProxyListMap.put(LogBoxLogLevel.Warn, new LogProxyList(LogBoxLogLevel.Warn));
    mNotification = new LogBoxNotification(context, this);
    registerLifeCycleListenerIfNeed(context);
  }

  private boolean registerLifeCycleListenerIfNeed(Context context) {
    if (!(context instanceof Activity) || ((Activity) context).isFinishing()) {
      return false;
    }
    Activity activity = (Activity) context;
    if (activity.getFragmentManager().findFragmentByTag(LIFE_CYCLE_LISTENER_FRAGMENT_TAG) != null) {
      return true;
    }
    try {
      activity.getFragmentManager()
          .beginTransaction()
          .add(new LogBoxLifeCycleListenerFragment(), LIFE_CYCLE_LISTENER_FRAGMENT_TAG)
          .commit();
    } catch (IllegalStateException e) {
      Log.w(TAG, "Failed to register Activity life cycle listener: " + e.getMessage());
      return false;
    }
    return true;
  }

  public void onNewLog(String msg, LogBoxLogLevel level, LogBoxProxy logBoxProxy) {
    if (logBoxProxy == null) {
      return;
    }
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return;
    }
    boolean isNewPoxy = false;
    proxyList.increaseLogCount();
    if (!proxyList.contains(logBoxProxy)) {
      proxyList.add(logBoxProxy);
      isNewPoxy = true;
    }
    if (mLogBox != null && mLogBox.isShowing()) {
      // mark should update notification when log box closing
      if (mNotification != null) {
        mNotification.invalidate(level);
      }
      if (mLogBox.getLevel() != level) {
        return;
      }
      if (proxyList.isCurrentProxy(logBoxProxy)) {
        mLogBox.showLogMessage(logBoxProxy.getErrorNamespace(), msg);
      } else if (isNewPoxy) {
        // if new log comes from a view that is not recorded
        // before by view list, update view count of log box
        mLogBox.updateViewInfo(proxyList.currentProxyIndex() + 1, proxyList.getProxiesCount(),
            level, getCurrentTemplateUrl(level));
      }
    } else if (mNotification != null && registerLifeCycleListenerIfNeed(mContext.get())) {
      // Method updateInfo of Notification would allocate UI resources lazily.
      // Before allocate UI resources, we should check if the activity life
      // cycle listener is registered or not to ensure the UI resources would
      // be released correctly when activity is destroying.
      String briefMessage = extractBriefMessage(msg);
      mNotification.updateInfo(level, briefMessage, proxyList.getLogCount());
    }
  }

  public void onNotificationClick(final LogBoxLogLevel level) {
    Context context = mContext.get();
    if (context == null) {
      return;
    }
    if (mLogBox == null) {
      mLogBox = new LogBoxDialog(context, this, new Runnable() {
        @Override
        public void run() {
          requestLogsOfCurrentView(level);
          if (mLogBox != null) {
            mLogBox.onLoadingFinished();
          }
        }
      });
    } else {
      requestLogsOfCurrentView(level);
    }
    if (context instanceof Activity) {
      if (!((Activity) context).isFinishing()) {
        mLogBox.showWithLevel(level);
      }
    }
  }

  public void onNotificationClose(LogBoxLogLevel level) {
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList != null) {
      proxyList.clearLogs(level);
    }
  }

  public void removeLogsOfCurrentView(LogBoxLogLevel level) {
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return;
    }
    LogBoxProxy proxy = proxyList.currentProxy();
    if (proxy != null) {
      // mark notification should update when log box dismiss
      if (mNotification != null) {
        mNotification.invalidate(level);
      }
      proxyList.removeProxy(proxy);
      proxy.clearLogsWithLevel(level);
    }
  }

  public void requestLogsOfCurrentView(LogBoxLogLevel level) {
    requestLogsOfViewIndex(LogProxyList.CURRENT_VIEW, level);
  }

  public void requestLogsOfViewIndex(int viewIndex, LogBoxLogLevel level) {
    Log.i(TAG, "show logs of view index " + viewIndex + " with level " + level);
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return;
    }
    if (viewIndex != LogProxyList.CURRENT_VIEW) {
      proxyList.setCurrentProxyIndex(viewIndex - 1);
    }
    if (mLogBox != null) {
      mLogBox.updateViewInfo(proxyList.currentProxyIndex() + 1, proxyList.getProxiesCount(), level,
          getCurrentTemplateUrl(level));
      mLogBox.setJSSource(getAllJsSourceOfCurrentView(level));
      LogBoxProxy proxy = proxyList.currentProxy();
      if (proxy != null) {
        mLogBox.showLogMessages(proxy.getErrorNamespace(), proxy.getLogMessages(level));
      }
    }
  }

  static protected String extractBriefMessage(String message) {
    if (TextUtils.isEmpty(message)) {
      return "";
    }
    String logStr = null;
    JSONObject rawLog = null;
    try {
      JSONObject logJson = new JSONObject(message);
      Object errorMessage = logJson.opt("error");
      if (errorMessage instanceof String) {
        logStr = (String) errorMessage;
        JSONObject errorJson = new JSONObject((String) errorMessage);
        rawLog = errorJson.optJSONObject("rawError");
      } else if (errorMessage instanceof JSONObject) {
        rawLog = ((JSONObject) errorMessage).optJSONObject("rawError");
      }
    } catch (JSONException e) {
      Log.i(TAG, "JSONException occurred when extract brief message of error: " + e.getMessage());
    }
    if (rawLog != null) {
      logStr = rawLog.optString("message", null);
    }
    if (logStr == null) {
      logStr = message;
    }
    if (logStr.length() > MAX_BRIEF_MSG_SIZE) {
      logStr = logStr.substring(0, MAX_BRIEF_MSG_SIZE);
    }
    logStr = logStr.replace('\n', ' ');
    return logStr;
  }

  public String getCurrentTemplateUrl(LogBoxLogLevel level) {
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return "";
    }
    LogBoxProxy proxy = proxyList.currentProxy();
    return proxy == null ? "" : proxy.getEntryUrlForLogSrc();
  }

  public Map<String, Object> getAllJsSourceOfCurrentView(LogBoxLogLevel level) {
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return null;
    }
    LogBoxProxy proxy = proxyList.currentProxy();
    return proxy == null ? null : proxy.getLogSources();
  }

  public String getLogSourceWithFileName(String fileName, LogBoxLogLevel level) {
    LogProxyList proxyList = mLogProxyListMap.get(level);
    if (proxyList == null) {
      return "";
    }
    LogBoxProxy proxy = proxyList.currentProxy();
    if (proxy == null) {
      return "";
    }
    return proxy.getLogSourceWithFileName(fileName);
  }

  public void onProxyReset(LogBoxProxy proxy) {
    if (proxy == null) {
      return;
    }
    for (Map.Entry<LogBoxLogLevel, LogProxyList> entry : mLogProxyListMap.entrySet()) {
      LogProxyList list = entry.getValue();
      if (list.removeProxy(proxy) && mNotification != null) {
        // mark notification should be update
        mNotification.invalidate(entry.getKey());
      }
    }
    if (mLogBox != null && mLogBox.isShowing()) {
      mLogBox.reset();
      mLogBox.dismiss();
    } else {
      updateNotificationIfNeed();
    }
    proxy.clearAllLogs();
  }

  public void onLogBoxDismiss() {
    updateNotificationIfNeed();
  }

  public void dismissDialog() {
    if (mLogBox != null) {
      mLogBox.reset();
      mLogBox.dismiss();
    }
  }

  private void updateNotificationIfNeed() {
    for (Map.Entry<LogBoxLogLevel, LogProxyList> entry : mLogProxyListMap.entrySet()) {
      LogProxyList proxyList = entry.getValue();
      if (mNotification != null && mNotification.isDirty(entry.getKey())
          && registerLifeCycleListenerIfNeed(mContext.get())) {
        // Method updateInfo of Notification would allocate UI resources lazily.
        // Before allocate UI resources, we should check if the activity life
        // cycle listener is registered or not to ensure the UI resources would
        // be released correctly when activity is destroying.
        String briefMsg = extractBriefMessage(proxyList.getLastLog());
        mNotification.updateInfo(entry.getKey(), briefMsg, proxyList.getLogCount());
      }
    }
  }

  public void destroy() {
    if (mLogBox != null) {
      mLogBox.dismiss();
      mLogBox = null;
    }
    if (mNotification != null) {
      mNotification.destroy();
      mNotification = null;
    }
  }

  private class LogProxyList {
    public final static int CURRENT_VIEW = -1;
    private final LogBoxLogLevel mLevel;
    private final List<LogBoxProxy> mProxyList;
    private int mCurrentProxyIndex;
    private int mLogCount;

    public LogProxyList(LogBoxLogLevel level) {
      mLevel = level;
      mProxyList = new ArrayList<>();
      mCurrentProxyIndex = 0;
      mLogCount = 0;
    }

    public LogBoxLogLevel getLevel() {
      return mLevel;
    }

    public boolean isCurrentProxy(LogBoxProxy proxy) {
      if (proxy != null) {
        int index = mProxyList.indexOf(proxy);
        if (index != -1 && index == mCurrentProxyIndex) {
          return true;
        }
      }
      return false;
    }

    public void increaseLogCount() {
      ++mLogCount;
    }

    public boolean contains(LogBoxProxy proxy) {
      return mProxyList.contains(proxy);
    }

    public boolean add(LogBoxProxy proxy) {
      return mProxyList.add(proxy);
    }

    public int getLogCount() {
      return mLogCount;
    }

    public int getProxiesCount() {
      return mProxyList.size();
    }

    public int currentProxyIndex() {
      Log.i(TAG, "current proxy index is " + mCurrentProxyIndex);
      return mCurrentProxyIndex;
    }

    public boolean setCurrentProxyIndex(int index) {
      if (index >= 0 && index < mProxyList.size()) {
        Log.i(TAG, "set current proxy index " + index);
        mCurrentProxyIndex = index;
        return true;
      }
      return false;
    }

    public LogBoxProxy currentProxy() {
      if (mCurrentProxyIndex >= mProxyList.size() || mCurrentProxyIndex < 0) {
        Log.e(TAG, "current proxy index out of bounds");
        return null;
      }
      return mProxyList.get(mCurrentProxyIndex);
    }

    public boolean removeProxy(LogBoxProxy proxy) {
      if (proxy == null || !mProxyList.contains(proxy)) {
        return false;
      }
      int logCount = proxy.getLogCount(mLevel);
      mLogCount -= logCount;
      mProxyList.remove(proxy);
      mCurrentProxyIndex = mProxyList.isEmpty() ? 0 : mCurrentProxyIndex % mProxyList.size();
      return true;
    }

    public void clearLogs(LogBoxLogLevel level) {
      for (LogBoxProxy proxy : mProxyList) {
        proxy.clearLogsWithLevel(level);
      }
      mProxyList.clear();
      mLogCount = 0;
      mCurrentProxyIndex = 0;
    }

    public String getLastLog() {
      if (mProxyList.isEmpty()) {
        return "";
      }
      LogBoxProxy proxy = mProxyList.get(mProxyList.size() - 1);
      return proxy.getLastLog(mLevel);
    }
  }
}
