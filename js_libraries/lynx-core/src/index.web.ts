// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
export { default as nativeGlobal } from './common/nativeGlobal';
export {
  loadCard,
  destroyCard,
  callDestroyLifetimeFun,
  loadDynamicComponent,
} from './appManager';
