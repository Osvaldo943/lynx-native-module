// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { AnimationV2 as IAnimation } from '@lynx-js/types';
import { KeyframeEffectV2 } from './effect';

export class AnimationV2 implements IAnimation {
  public readonly effect: KeyframeEffectV2;
  public readonly id: string;

  constructor(
    id: string,
    keyframes: Array<Record<string, any>>,
    options: Record<string, any>
  ) {
    this.id = id;
    this.effect = new KeyframeEffectV2(keyframes, options);
  }
}
