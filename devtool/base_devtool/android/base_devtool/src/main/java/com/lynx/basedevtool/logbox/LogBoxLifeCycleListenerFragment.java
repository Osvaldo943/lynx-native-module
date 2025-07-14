// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.app.Fragment;
import android.content.Context;
import android.util.Log;

// A empty fragment be used to listen the onDestory event of activity.
// When the activity is destroying, release resources like dialogs and views
// that hold the activity in LogBoxManager to avoid memory leak.
public class LogBoxLifeCycleListenerFragment extends Fragment {
  private static final String TAG = "LogBoxLifeCycleListener";

  @Override
  public void onDestroy() {
    Log.i(TAG, "on destroy");
    Context activity = getActivity();
    LogBoxManager manager = LogBoxOwner.getInstance().findManagerByActivityIfExist(activity);
    if (manager != null) {
      manager.destroy();
    }
    super.onDestroy();
  }
}
