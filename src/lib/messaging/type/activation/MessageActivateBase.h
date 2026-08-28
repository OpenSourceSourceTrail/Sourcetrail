#pragma once
// internal
#include "data/search/SearchMatch.h"

class MessageActivateBase {
public:
  virtual ~MessageActivateBase() = default;

  virtual std::vector<SearchMatch> getSearchMatches() const = 0;
};