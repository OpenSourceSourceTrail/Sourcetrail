#pragma once
#include <ranges>

namespace utility {

template <typename Container>
struct ToContainerFn {
  // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): only iterated, never forwarded.
  template <std::ranges::input_range R>
  friend Container operator|(R&& range, ToContainerFn /*unused*/) {
    Container container;
    for(auto&& value : range) {
      container.insert(container.end(), value);
    }
    return container;
  }
};

template <typename Container>
constexpr ToContainerFn<Container> toContainer() noexcept {
  return {};
}

}    // namespace utility
