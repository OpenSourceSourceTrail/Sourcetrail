#pragma once
#include <vector>

struct ErrorInfo;

struct ErrorCountInfo {
  ErrorCountInfo();

  ErrorCountInfo(size_t total_, size_t fatal_);

  explicit ErrorCountInfo(const std::vector<ErrorInfo>& errors);

  size_t total = 0;
  size_t fatal = 0;
};

bool operator==(const ErrorCountInfo& item0, const ErrorCountInfo& item1);
