// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.base.LLog;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderReplayDataModule extends LynxModule {
  private static final String TAG = "LynxRecorderReplayDataModule";
  private JSONArray mFunctionCall;
  private JSONObject mCallbackData;
  private JSONArray mJsbIgnoredInfo;
  private JSONObject mJsbSettings;

  public LynxRecorderReplayDataModule(Context context) {
    super(context);
  }

  public LynxRecorderReplayDataModule(Context context, Object param) {
    super(context);
    LynxRecorderReplayDataProvider provider = (LynxRecorderReplayDataProvider) param;
    mFunctionCall = provider.getFunctionCall();
    mCallbackData = provider.getCallbackData();
    mJsbIgnoredInfo = provider.getJsbIgnoredInfo();
    mJsbSettings = provider.getJsbSettings();
  }

  @LynxMethod
  public void getData(final Callback callback) {
    JSONObject data = new JSONObject();
    try {
      data.put("RecordData", getRecordData());
      data.put("JsbIgnoredInfo", getJsbIgnoredInfo());
      data.put("JsbSettings", getJsbSettings());
    } catch (JSONException e) {
      LLog.e("TestBench", "Record file format error!");
      callback.invoke("{}");
    }
    callback.invoke(data.toString());
  }

  private String getRecordData() {
    JSONObject json = new JSONObject();
    try {
      for (int index = 0; index < mFunctionCall.length(); index++) {
        JSONObject funcInvoke = mFunctionCall.getJSONObject(index);
        String moduleName = funcInvoke.getString("Module Name");
        if (!json.has(moduleName)) {
          json.put(moduleName, new JSONArray());
        }

        JSONObject methodLookUp = new JSONObject();

        String methodName = funcInvoke.getString("Method Name");

        long requestTime = Long.parseLong(funcInvoke.getString("Record Time")) * 1000;
        if (funcInvoke.has("RecordMillisecond")) {
          requestTime = funcInvoke.getLong("RecordMillisecond");
        }
        JSONObject params = funcInvoke.getJSONObject("Params");

        JSONArray callbackIDs;
        try {
          callbackIDs = params.getJSONArray("callback");
        } catch (JSONException e) {
          callbackIDs = null;
        }
        StringBuffer functionInvokeLabel = new StringBuffer();
        JSONArray callbackReturnValues = new JSONArray();
        if (callbackIDs != null) {
          for (int i = 0; i < callbackIDs.length(); i++) {
            JSONObject callbackInfo;
            try {
              callbackInfo = mCallbackData.getJSONObject(callbackIDs.getString(i));
            } catch (JSONException e) {
              callbackInfo = null;
            }
            if (callbackInfo != null) {
              // get callback response Time
              long responseTime = Long.parseLong(callbackInfo.getString("Record Time")) * 1000;
              if (funcInvoke.has("RecordMillisecond")) {
                responseTime = callbackInfo.getLong("RecordMillisecond");
              }
              JSONObject callbackKernel = new JSONObject();
              callbackKernel.put("Value", callbackInfo.getJSONObject("Params"));
              callbackKernel.put("Delay", responseTime - requestTime);
              callbackReturnValues.put(i, callbackKernel);
            }
            functionInvokeLabel.append(callbackIDs.getString(i)).append("_");
          }
        }

        if (funcInvoke.has("SyncAttributes")) {
          methodLookUp.put("SyncAttributes", funcInvoke.getJSONObject("SyncAttributes"));
        }

        methodLookUp.put("Method Name", methodName);
        methodLookUp.put("Params", params);
        methodLookUp.put("Callback", callbackReturnValues);
        methodLookUp.put("Label", functionInvokeLabel.toString());

        json.getJSONArray(moduleName).put(methodLookUp);
      }
    } catch (JSONException e) {
      LLog.e("TestBench", "Record file format error!");
      return "{}";
    }
    return json.toString();
  }

  private String getJsbIgnoredInfo() {
    if (mJsbIgnoredInfo != null) {
      return mJsbIgnoredInfo.toString();
    } else {
      LLog.e("TestBench",
          "getJsbIgnoredInfo "
              + " error: download File failed");
      return "[]";
    }
  }

  private String getJsbSettings() {
    if (mJsbSettings != null) {
      return mJsbSettings.toString();
    } else {
      LLog.e("TestBench",
          "getJsbSettings "
              + " error: download File failed");
      return "{}";
    }
  }
}
