#pragma once
#include <string>
#include <utility>

enum StatusType {
  STATUS_INFO = 1,
  STATUS_ERROR = 2,
};

using StatusFilter = int;

struct Status final {
  Status(std::wstring message_, bool isError_ = false);

  std::wstring message;
  StatusType type;
};