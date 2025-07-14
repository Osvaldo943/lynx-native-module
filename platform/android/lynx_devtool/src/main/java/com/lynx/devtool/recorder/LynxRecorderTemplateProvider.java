// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import android.os.Handler;
import android.os.Looper;
import com.lynx.tasm.provider.AbsTemplateProvider;
import java.io.IOException;
import okhttp3.Call;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

public class LynxRecorderTemplateProvider extends AbsTemplateProvider {
  private final OkHttpClient client = new OkHttpClient();
  private final Handler mainHandler = new Handler(Looper.getMainLooper());
  @Override
  public void loadTemplate(String url, final Callback callback) {
    Request request = new Request.Builder().url(url).build();

    client.newCall(request).enqueue(new okhttp3.Callback() {
      @Override
      public void onFailure(Call call, IOException e) {
        mainHandler.post(() -> { callback.onFailed(e.toString()); });
      }

      @Override
      public void onResponse(Call call, Response response) throws IOException {
        if (!response.isSuccessful()) {
          IOException exception = new IOException("Unexpected code: " + response);
          mainHandler.post(() -> { callback.onFailed(exception.toString()); });
          return;
        }
        byte[] responseData = response.body().bytes();
        mainHandler.post(() -> { callback.onSuccess(responseData); });
      }
    });
  }
}
