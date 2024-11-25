#pragma once
#include <cstddef>

using Id = size_t;    // TODO(Hussein): Replace it with GlobalId

struct GlobalId final {
  constexpr GlobalId() noexcept = default;
  constexpr explicit GlobalId(size_t index) noexcept : mValue(index) {}

  constexpr operator bool() const noexcept {
    return mInitialized;
  }

  [[nodiscard]] constexpr bool isValid() const noexcept {
    return mInitialized;
  }

  constexpr operator size_t() const noexcept {
    return mValue;
  }

  [[nodiscard]] constexpr size_t value() const noexcept {
    return mValue;
  }

  [[nodiscard]] constexpr bool operator==(GlobalId other) const noexcept {
    return mInitialized == other.mInitialized && mValue == other.mValue;
  }

  [[nodiscard]] constexpr bool operator<(GlobalId other) const noexcept {
    return mInitialized == other.mInitialized && mValue < other.mValue;
  }

  [[nodiscard]] constexpr bool operator==(Id other) const noexcept {
    return mValue == other;
  }

private:
  bool mInitialized = false;
  size_t mValue = 0;
};

[[nodiscard]] inline constexpr bool operator==(Id other, GlobalId id) noexcept {
  return id == other;
}

[[nodiscard]] inline constexpr bool operator<(Id other, GlobalId id) noexcept {
  return id.value() < other;
}

inline constexpr GlobalId operator"" _gi(unsigned long long int index) noexcept {
  return GlobalId{index};
}