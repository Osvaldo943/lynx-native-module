// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_SVG_PATH_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_SVG_PATH_UTILS_H_

#include <cmath>
#include <sstream>
namespace lynx {
namespace tasm {
namespace harmony {
class SvgPathUtils {
 public:
  // Move to (M)
  static inline void MoveTo(std::ostringstream& oss, float x, float y) {
    oss << "M " << x << " " << y << " ";
  }

  // Line to (L)
  static inline void LineTo(std::ostringstream& oss, float x, float y) {
    oss << "L " << x << " " << y << " ";
  }

  // Arc to with endpoint (simplified A rx ry 0 0 1 x y)
  static inline void ArcTo(std::ostringstream& oss, float rx, float ry, float x,
                           float y) {
    oss << "A " << rx << " " << ry << " 0 0 1 " << x << " " << y << " ";
  }

  // Arc to with center box and angles (full form)
  static inline void ArcTo(std::ostringstream& oss, float rx, float ry,
                           float left, float top, float right, float bottom,
                           float start_deg, float sweep_deg) {
    float cx = (left + right) / 2.0f;
    float cy = (top + bottom) / 2.0f;
    double end_radians = (start_deg + sweep_deg) * M_PI / 180.0;
    float x = cx + rx * std::cos(end_radians);
    float y = cy + ry * std::sin(end_radians);
    ArcTo(oss, rx, ry, x, y);
  }

  // Close path (Z)
  static inline void Close(std::ostringstream& oss) { oss << "Z"; }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_SVG_PATH_UTILS_H_
