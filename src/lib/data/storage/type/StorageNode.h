#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageNodeData {
  StorageNodeData() : type(0), serializedName(L"") {}

  StorageNodeData(int type_, std::wstring serializedName_) : type(type_), serializedName(std::move(serializedName_)) {}

  bool operator<(const StorageNodeData& other) const {
    return serializedName < other.serializedName;
  }

  int type = 0;
  std::wstring serializedName = {};
};

struct StorageNode : public StorageNodeData {
  StorageNode() : StorageNodeData() {}

  StorageNode(Id id_, int type_, std::wstring serializedName_) : StorageNodeData(type_, std::move(serializedName_)), id(id_) {}

  StorageNode(Id id_, const StorageNodeData& data_) : StorageNodeData(data_), id(id_) {}

  Id id = 0;
};
