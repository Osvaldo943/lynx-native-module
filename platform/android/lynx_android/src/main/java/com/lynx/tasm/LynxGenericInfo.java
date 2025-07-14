// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.eventreport.LynxEventReporter.PROP_NAME_RELATIVE_PATH;
import static com.lynx.tasm.eventreport.LynxEventReporter.PROP_NAME_URL;

import android.net.Uri;
import android.text.TextUtils;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import java.io.File;
import java.util.HashSet;
import java.util.Set;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Class hold some info like templateURL.
 * It's used to report some common useful parameter when report event.
 */
public class LynxGenericInfo {
  private static final String TAG = "LynxGenericInfo";

  // GeneralInfo props value:
  private String mPropValueURL;
  private String mPropValueRelativePath;

  private static final Set<String> mReservedQueryKeys = new HashSet<String>();

  static {
    mReservedQueryKeys.add("url");
    mReservedQueryKeys.add("surl");
    mReservedQueryKeys.add("channel");
    mReservedQueryKeys.add("bundle");
  }

  // TODO(kechenglong): Remove this class.
  public LynxGenericInfo() {}

  // Only used for testing by now.
  public JSONObject toJSONObject() {
    JSONObject ret = new JSONObject();
    try {
      ret.putOpt(PROP_NAME_URL, mPropValueURL);
      ret.putOpt(PROP_NAME_RELATIVE_PATH, mPropValueRelativePath);
    } catch (JSONException e) {
      LLog.w(TAG, "LynxGenericInfo toJSONObject failed");
      e.printStackTrace();
    }
    return ret;
  }

  // URL Info
  public void updateLynxUrl(LynxContext lynxContext, String templateURL) {
    if (TextUtils.isEmpty(templateURL)) {
      return;
    }
    if (TextUtils.equals(templateURL, mPropValueURL)) {
      return;
    }
    mPropValueURL = templateURL;
    TraceEvent.beginSection(TraceEventDef.GENERIC_INFO_UPDATE_RELATIVE_URL);
    updateRelativeURL(lynxContext);
    TraceEvent.endSection(TraceEventDef.GENERIC_INFO_UPDATE_RELATIVE_URL);
  }

  public String getPropValueRelativePath() {
    return this.mPropValueRelativePath;
  }

  // get and cache applicationExternalCacheDir
  private static String applicationExternalCacheDir;
  private void getApplicationExternalCacheDir(LynxContext lynxContext) {
    if (applicationExternalCacheDir != null && !applicationExternalCacheDir.isEmpty()) {
      // if applicationExternalCacheDir has been gotten before, just use this value
      return;
    }
    if (lynxContext != null) {
      File externalCacheDir = lynxContext.getExternalCacheDir();
      if (externalCacheDir != null) {
        applicationExternalCacheDir = externalCacheDir.getPath();
      }
    }
  }

  // get and cache applicationFilesDir
  private static String applicationFilesDir;
  private void getApplicationFilesDir(LynxContext lynxContext) {
    if (applicationFilesDir != null && !applicationFilesDir.isEmpty()) {
      // if applicationFilesDir has been gotten before, just use this value
      return;
    }
    if (lynxContext != null) {
      File filesDir = lynxContext.getFilesDir();
      if (filesDir != null) {
        applicationFilesDir = filesDir.getPath();
      }
    }
  }

  /**
   * Remove redundant query parameters from relativePath and keep reserved query parameters
   * @param relativePath relative url
   * @return normalized relative url
   */
  private String removeRedundantQueryParams(String relativePath) {
    if (TextUtils.isEmpty(relativePath)) {
      return relativePath;
    }

    String finalUrl = relativePath;
    // try removing redundant query parmeters
    // e.g. https://lf-ecom-xx/obj/xxx/template.js?enter_from=XXX
    try {
      Uri uri = Uri.parse(relativePath);
      // Remove redundant query parameters to improve clustering accuracy
      if (uri.isHierarchical()) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme(uri.getScheme())
            .encodedAuthority(uri.getEncodedAuthority())
            .encodedPath(uri.getEncodedPath());
        for (String query : mReservedQueryKeys) {
          String parameter = uri.getQueryParameter(query);
          if (!TextUtils.isEmpty(parameter)) {
            builder.appendQueryParameter(query, parameter);
          }
        }
        finalUrl = builder.toString();
      }
    } catch (NullPointerException | UnsupportedOperationException | IllegalArgumentException
        | IndexOutOfBoundsException e) {
      LLog.w(TAG, "Parsing hierarchical schema failed for url is null with " + e.getMessage());
    }

    return finalUrl;
  }

  private void updateRelativeURL(LynxContext lynxContext) {
    mPropValueRelativePath = mPropValueURL;
    getApplicationExternalCacheDir(lynxContext);
    getApplicationFilesDir(lynxContext);
    // try removing applicationExternalCacheDir and applicationFilesDir
    if (applicationExternalCacheDir != null && !applicationExternalCacheDir.isEmpty()) {
      mPropValueRelativePath = mPropValueRelativePath.replace(applicationExternalCacheDir, "");
    }
    if (applicationFilesDir != null && !applicationFilesDir.isEmpty()) {
      mPropValueRelativePath = mPropValueRelativePath.replace(applicationFilesDir, "");
    }

    mPropValueRelativePath = removeRedundantQueryParams(mPropValueRelativePath);
  }
}
