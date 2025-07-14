// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.content.Context;
import android.text.TextUtils;
import com.lynx.basedevtool.utils.DevToolFileLoadCallback;
import java.util.HashMap;
import java.util.Map;

class LogBoxEnv {
  private static final String TAG = "LogBoxEnv";
  private final Map<String, ILogBoxErrorParserLoader> mErrorParserLoaders;

  public interface ILogBoxErrorParserLoader {
    void loadErrorParser(Context context, DevToolFileLoadCallback callback);
  }

  public static LogBoxEnv inst() {
    return SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final LogBoxEnv INSTANCE = new LogBoxEnv();
  }

  private LogBoxEnv() {
    mErrorParserLoaders = new HashMap<>();
  }

  public void registerErrorParserLoader(String namespace, ILogBoxErrorParserLoader loader) {
    if (TextUtils.isEmpty(namespace) || mErrorParserLoaders.get(namespace) != null
        || loader == null) {
      return;
    }
    synchronized (mErrorParserLoaders) {
      if (mErrorParserLoaders.get(namespace) == null) {
        mErrorParserLoaders.put(namespace, loader);
      }
    }
  }

  protected void loadErrorParser(
      Context context, String namespace, DevToolFileLoadCallback callback) {
    ILogBoxErrorParserLoader loader = mErrorParserLoaders.get(namespace);
    if (loader != null) {
      loader.loadErrorParser(context, callback);
    }
  }
}
