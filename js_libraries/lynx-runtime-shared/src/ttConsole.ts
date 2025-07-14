// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { isFunction } from './utils';

export type SharedConsole = typeof nativeConsole & { runtimeId: string };

/**
 * Create a console that wrapped the nativeConsole to log with runtimeId.
 * @param runtimeId The runtimeId to be logged
 *
 * The runtimeId can be changed by setting directly.
 *
 * @example
 * const sharedConsole = createSharedConsole(runtimeId);
 */
export function createSharedConsole(runtimeId?: string): SharedConsole {
  // TODO(zhangqun.29): Delete all references to runtimeId
  return nativeConsole as SharedConsole;
}

const _global = (function () {
  // eslint-disable-next-line no-eval
  return this || (0, eval)('this');
})();

/**
 * This is a wrapper to nativeConsole that log with groupId.
 *
 * The groupId defaults to '-1' and can be changed.
 */
const groupConsole = createSharedConsole(`groupId:${_global.groupId || '-1'}`);

/**
 * All console in lynx-kernel should use this console
 */
export default NODE_ENV === 'development'
  ? groupConsole
  : (nativeConsole as SharedConsole);
