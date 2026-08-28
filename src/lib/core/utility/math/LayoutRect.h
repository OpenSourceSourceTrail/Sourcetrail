#pragma once

namespace core {

// Axis-aligned bounding box stored as two corners: {left, top, right, bottom}.
// Qt-free replacement for QVector4D used in graph layout code.
// Convert to/from QVector4D only at the lib_gui boundary.
struct LayoutRect {
  float left = 0.0f;
  float top = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;

  [[nodiscard]] float width() const noexcept {
    return right - left;
  }
  [[nodiscard]] float height() const noexcept {
    return bottom - top;
  }

  // True when the rect encloses a non-empty area (bottom > 0 or right > left).
  [[nodiscard]] bool isValid() const noexcept {
    return bottom > 0.0f;
  }

  [[nodiscard]] bool operator==(const LayoutRect&) const noexcept = default;
};

}    // namespace core

using LayoutRect = core::LayoutRect;
