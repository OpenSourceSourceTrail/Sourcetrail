#pragma once
// STL
#include <vector>
// internal
#include "ErrorInfo.h"

struct ErrorFilter {
  ErrorFilter() = default;

  bool filter(const ErrorInfo& info) const {
    if(!error && !info.fatal && info.indexed)
      return false;
    if(!fatal && info.fatal && info.indexed)
      return false;
    if(!unindexedError && !info.fatal && !info.indexed)
      return false;
    if(!unindexedFatal && info.fatal && !info.indexed)
      return false;
    return true;
  }

  std::vector<ErrorInfo> filterErrors(const std::vector<ErrorInfo>& errors) const {
    std::vector<ErrorInfo> filteredErrors;

    for(const auto& err : errors) {
      if(filter(err)) {
        filteredErrors.push_back(err);

        if(limit > 0 && filteredErrors.size() >= limit) {
          break;
        }
      }
    }

    return filteredErrors;
  }

  bool operator==(const ErrorFilter& other) const {
    return error == other.error && fatal == other.fatal && unindexedError == other.unindexedError &&
        unindexedFatal == other.unindexedFatal && limit == other.limit;
  }

  bool error = true;
  bool fatal = true;

  bool unindexedError = true;
  bool unindexedFatal = true;

  size_t limit = 100;
};
