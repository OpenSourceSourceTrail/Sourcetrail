#pragma once
#include <type_traits>

namespace utility {

template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
constexpr auto to_underlying(T type) noexcept {
  return static_cast<typename std::underlying_type_t<T>>(type);
}

}    // namespace utility
