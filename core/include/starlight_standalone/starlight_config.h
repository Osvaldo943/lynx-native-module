// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_CONFIG_H_
#define CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_CONFIG_H_

#include "starlight.h"
#include "starlight_value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarlightConfig StarlightConfig;

StarlightConfig* SLConfigCreate();

void SLConfigSetPhysicalPixelsPerLayoutUnit(
    StarlightConfig* const config, float physical_pixels_per_layout_unit);

void SLConfigFree(const StarlightConfig* config);

float SLConfigGetPhysicalPixelsPerLayoutUnit(const StarlightConfig* config);

#ifdef __cplusplus
}
#endif

#endif  // CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_CONFIG_H_
