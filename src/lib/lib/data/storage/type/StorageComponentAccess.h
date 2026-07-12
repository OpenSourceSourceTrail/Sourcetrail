#pragma once
// internal
#include "GlobalId.hpp"

struct StorageComponentAccess {
  StorageComponentAccess() = default;

  StorageComponentAccess(Id nodeId_, int type_) : nodeId(nodeId_), type(type_) {}

  bool operator<(const StorageComponentAccess& other) const {
    return nodeId < other.nodeId;
  }

  Id nodeId = 0;
  int type = 0;
};
