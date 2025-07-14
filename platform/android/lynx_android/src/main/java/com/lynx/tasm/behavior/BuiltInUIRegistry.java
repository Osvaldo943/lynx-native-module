// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.image.FlattenUIImage;
import com.lynx.tasm.behavior.ui.image.InlineImageShadowNode;
import com.lynx.tasm.behavior.ui.image.UIImage;
import com.lynx.tasm.image.AutoSizeImage;
import java.util.HashMap;
import java.util.Map;

/**
 * Registry for built-in UI components and their behaviors.
 *
 */
public class BuiltInUIRegistry {
  // Pre-defined component tags
  private static final String IMAGE_TAG = "image";
  private static final String INLINE_IMAGE_TAG = "inline-image";

  private static class BehaviorHolder {
    static final Map<String, Behavior> mBehaviorsMap = new HashMap<>();
    static {
      // image
      mBehaviorsMap.put(IMAGE_TAG, new Behavior(IMAGE_TAG, true, true) {
        @Override
        public LynxUI createUI(LynxContext context) {
          return new UIImage(context);
        }

        @Override
        public LynxFlattenUI createFlattenUI(LynxContext context) {
          return new FlattenUIImage(context);
        }

        @Override
        public ShadowNode createShadowNode() {
          return new AutoSizeImage();
        }
      });

      // inline-image
      mBehaviorsMap.put(INLINE_IMAGE_TAG, new Behavior(INLINE_IMAGE_TAG, false, true) {
        @Override
        public ShadowNode createShadowNode() {
          return new InlineImageShadowNode();
        }
      });
    }
  }

  private static class Holder {
    static final BuiltInUIRegistry INSTANCE = new BuiltInUIRegistry();
  }

  public static BuiltInUIRegistry getInstance() {
    return Holder.INSTANCE;
  }

  /**
   * Retrieves the map of built-in UI behaviors
   * @return Unmodifiable map of component behaviors
   */
  public Map<String, Behavior> getBuiltInUIBehaviors() {
    return BehaviorHolder.mBehaviorsMap;
  }
}
