// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;

import com.lynx.react.bridge.JavaOnlyMap;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;

public class LynxGenericInfoTest {
  @Test
  public void updateLynxUrl() {
    String[] originalUrls = new String[] {// absolute urls
        "https://obj/template.js?channel=test_channel&abkeys=s_optimize",
        "https://obj/template.js?bundle=test_channel&abkeys=s_optimize",
        "https://obj/template.js?url=test_channel&abkeys=s_optimize",
        "https://obj/template.js?surl=test_channel&abkeys=s_optimize",
        // relative urls
        "this is a lynx url"};

    String[] expectedRelativeUrls = new String[] {"https://obj/template.js?channel=test_channel",
        "https://obj/template.js?bundle=test_channel", "https://obj/template.js?url=test_channel",
        "https://obj/template.js?surl=test_channel", "this is a lynx url"};

    for (int caseIdx = 0; caseIdx < originalUrls.length; caseIdx++) {
      LynxGenericInfo lynxGenericInfo = new LynxGenericInfo();
      lynxGenericInfo.updateLynxUrl(null, originalUrls[caseIdx]);
      JSONObject data = lynxGenericInfo.toJSONObject();
      try {
        assertEquals(data.getString("url"), originalUrls[caseIdx]);
        assertEquals(data.getString("relative_path"), expectedRelativeUrls[caseIdx]);
      } catch (JSONException e) {
        fail();
      }
    }

    // Test null corner case
    LynxGenericInfo lynxGenericInfo = new LynxGenericInfo();
    lynxGenericInfo.updateLynxUrl(null, null);
    JSONObject data = lynxGenericInfo.toJSONObject();
    assertFalse(data.has("url"));
    assertFalse(data.has("relative_path"));
  }
}
