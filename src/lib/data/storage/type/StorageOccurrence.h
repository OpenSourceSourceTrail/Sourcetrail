#pragma once
// internal
#include "GlobalId.hpp"

struct StorageOccurrence {
  StorageOccurrence() = default;

  StorageOccurrence(Id elementId_, Id sourceLocationId_) : elementId(elementId_), sourceLocationId(sourceLocationId_) {}

  bool operator<(const StorageOccurrence& other) const {
    if(elementId != other.elementId) {
      return elementId < other.elementId;
    } else {
      return sourceLocationId < other.sourceLocationId;
    }
  }

  Id elementId = 0;
  Id sourceLocationId = 0;
};
