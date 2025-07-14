// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper;

import com.lynx.basedevtool.utils.DevToolDownloader;
import com.lynx.basedevtool.utils.DownloadCallback;
import com.lynx.tasm.base.LLog;
import java.nio.charset.Charset;
import java.util.concurrent.atomic.AtomicBoolean;

public class LepusDebugInfoHelper {
  private static final String TAG = "LepusDebugInfoHelper";

  private AtomicBoolean mIsLoading;
  private String mDebugInfo;

  public LepusDebugInfoHelper() {
    mIsLoading = new AtomicBoolean(false);
  }

  public String getDebugInfo(String url) {
    LLog.i(TAG, "lepus debug: debug info url: " + url);
    mIsLoading.set(true);
    downloadDebugInfo(url);

    try {
      // Wait for downloading
      while (mIsLoading.get()) {
        Thread.sleep(10);
      }
    } catch (InterruptedException e) {
      LLog.e(TAG, e.toString());
    }

    return mDebugInfo;
  }

  private void downloadDebugInfo(String url) {
    new DevToolDownloader(url, new DownloadCallback() {
      @Override
      public void onResponse(int status, int contentLength) {}
      @Override
      public void onData(byte[] bytes, int length) {
        mDebugInfo = new String(bytes, Charset.defaultCharset());
        mIsLoading.set(false);
      }
      @Override
      public void onFailure(String reason) {
        mDebugInfo = "";
        mIsLoading.set(false);
        LLog.e(TAG, "lepus debug: download debug info failed, the reason is: " + reason);
      }
    });
  }
}
