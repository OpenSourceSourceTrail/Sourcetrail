#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageElementComponent {
  StorageElementComponent() = default;

  StorageElementComponent(Id elementId_, int type_, std::wstring data_)
      : elementId(elementId_), type(type_), data(std::move(data_)) {}

  bool operator<(const StorageElementComponent& other) const {
    if(elementId != other.elementId) {
      return elementId < other.elementId;
    } else if(type != other.type) {
      return type < other.type;
    } else {
      return data < other.data;
    }
  }

  Id elementId = 0;
  int type = 0;
  std::wstring data = {};
};
