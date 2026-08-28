#pragma once

namespace core {

// Lightweight 2-D float vector — Qt-free replacement for QVector2D.
// Fields are public so aggregate initialisation and designated initialisers work:
//   Vec2f p{.x = 3.0f, .y = 4.0f};
// Convert to/from QVector2D only at the lib_gui boundary.
struct Vec2f {
  float x = 0.0f;
  float y = 0.0f;

  Vec2f& operator+=(const Vec2f& o) noexcept {
    x += o.x;
    y += o.y;
    return *this;
  }
  Vec2f& operator-=(const Vec2f& o) noexcept {
    x -= o.x;
    y -= o.y;
    return *this;
  }
  [[nodiscard]] Vec2f operator+(const Vec2f& o) const noexcept {
    return {x + o.x, y + o.y};
  }
  [[nodiscard]] Vec2f operator-(const Vec2f& o) const noexcept {
    return {x - o.x, y - o.y};
  }
  [[nodiscard]] Vec2f operator*(float s) const noexcept {
    return {x * s, y * s};
  }
  [[nodiscard]] Vec2f operator/(float s) const noexcept {
    return {x / s, y / s};
  }
  [[nodiscard]] Vec2f operator-() const noexcept {
    return {-x, -y};
  }

  // Indexed access: 0 → x, 1 → y (mirrors QVector2D for layout code that uses xIdx/yIdx)
  [[nodiscard]] float operator[](int i) const noexcept {
    return i == 0 ? x : y;
  }
  float& operator[](int i) noexcept {
    return i == 0 ? x : y;
  }

  [[nodiscard]] bool operator==(const Vec2f&) const noexcept = default;
};

}    // namespace core

using Vec2f = core::Vec2f;
