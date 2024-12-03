#pragma once
#include <cstdint>
#include <string>

enum class StatusType : std::uint8_t {
  None = 0,
  Info = 1,
  Error = 2,
};

using StatusFilter = std::underlying_type_t<StatusType>;

struct Status final {
  Status(std::wstring message_, bool isError_ = false);

  std::wstring message;
  StatusType type;
};
