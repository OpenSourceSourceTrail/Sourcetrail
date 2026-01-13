#pragma once
#include <type_traits>

namespace utility {

template <typename T>
constexpr auto to_underlying(T type) noexcept
  requires(std::is_enum_v<T>)
{
  return static_cast<typename std::underlying_type_t<T>>(type);
}

}    // namespace utility
