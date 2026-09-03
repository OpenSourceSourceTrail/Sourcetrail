#pragma once
// internal
#include "search/domain/SearchMatch.h"

class MessageActivateBase {
public:
  virtual ~MessageActivateBase() = default;

  virtual std::vector<SearchMatch> getSearchMatches() const = 0;
};