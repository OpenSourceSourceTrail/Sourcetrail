#pragma once
#include <utility>
#include <vector>

#include "TokenComponent.h"
#include "GlobalId.hpp"

class TokenComponentInheritanceChain : public TokenComponent {
public:
  explicit TokenComponentInheritanceChain(std::vector<Id> inheritanceEdgeIds_)
      : inheritanceEdgeIds(std::move(inheritanceEdgeIds_)) {}

  [[nodiscard]] std::shared_ptr<TokenComponent> copy() const override {
    return std::make_shared<TokenComponentInheritanceChain>(*this);
  }

  const std::vector<Id> inheritanceEdgeIds;
};
