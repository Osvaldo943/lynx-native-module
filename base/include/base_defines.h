// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BASE_DEFINES_H_
#define BASE_INCLUDE_BASE_DEFINES_H_

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

#if _MSC_VER
#define BASE_INLINE inline __forceinline
#else
#define BASE_INLINE inline __attribute__((always_inline))
#endif

#endif  // BASE_INCLUDE_BASE_DEFINES_H_
