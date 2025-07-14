// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/include/starlight_standalone/starlight_config.h"

struct StarlightConfig {
  float physical_pixels_per_layout_unit_;
};

StarlightConfig* SLConfigCreate() {
  StarlightConfig* config = new StarlightConfig();
  config->physical_pixels_per_layout_unit_ = 1.0f;
  return config;
}

void SLConfigSetPhysicalPixelsPerLayoutUnit(
    StarlightConfig* const config, float physical_pixels_per_layout_unit) {
  config->physical_pixels_per_layout_unit_ = physical_pixels_per_layout_unit;
}

float SLConfigGetPhysicalPixelsPerLayoutUnit(const StarlightConfig* config) {
  return config ? config->physical_pixels_per_layout_unit_ : 1.0f;
}

void SLConfigFree(StarlightConfig* config) { delete config; }
