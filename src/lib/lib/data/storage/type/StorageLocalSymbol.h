#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageLocalSymbolData {
  StorageLocalSymbolData() = default;

  StorageLocalSymbolData(std::wstring name_) : name(std::move(name_)) {}

  bool operator<(const StorageLocalSymbolData& other) const {
    return name < other.name;
  }

  std::wstring name = {};
};

struct StorageLocalSymbol : public StorageLocalSymbolData {
  StorageLocalSymbol() : StorageLocalSymbolData() {}

  StorageLocalSymbol(Id id_, const StorageLocalSymbolData& data_) : StorageLocalSymbolData(data_), id(id_) {}

  StorageLocalSymbol(Id id_, std::wstring name_) : StorageLocalSymbolData(std::move(name_)), id(id_) {}

  Id id = 0;
};
