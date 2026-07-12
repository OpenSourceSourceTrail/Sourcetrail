#pragma once
#include <cstddef>

using Id = size_t;    // TODO(Hussein): Replace it with GlobalId

struct GlobalId final {
  constexpr GlobalId() noexcept = default;
  constexpr GlobalId(size_t index) noexcept : mValue(index) {}    // NOLINT: After removing Id add explicit

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

[[nodiscard]] constexpr bool operator==(Id other, GlobalId id) noexcept {
  return id == other;
}

[[nodiscard]] constexpr bool operator<(Id other, GlobalId id) noexcept {
  return id.value() < other;
}

constexpr GlobalId operator""_gi(unsigned long long int index) noexcept {
  return GlobalId{index};
}
