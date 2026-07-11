#include "ErrorCountInfo.h"

#include <range/v3/algorithm/count_if.hpp>

#include "ErrorInfo.h"

ErrorCountInfo::ErrorCountInfo() = default;

ErrorCountInfo::ErrorCountInfo(size_t total_, size_t fatal_) : total(total_), fatal(fatal_) {}

ErrorCountInfo::ErrorCountInfo(const std::vector<ErrorInfo>& errors)
    : total{errors.size()}
    , fatal{static_cast<size_t>(ranges::count_if(errors, [](const ErrorInfo& error) { return error.fatal; }))} {}

bool operator==(const ErrorCountInfo& item0, const ErrorCountInfo& item1) {
  return item0.fatal == item1.fatal && item0.total == item1.total;
}
