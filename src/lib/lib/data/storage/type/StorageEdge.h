#pragma once
// internal
#include "GlobalId.hpp"

struct StorageEdgeData {
  StorageEdgeData() = default;

  StorageEdgeData(int type_, Id sourceNodeId_, Id targetNodeId_)
      : type(type_), sourceNodeId(sourceNodeId_), targetNodeId(targetNodeId_) {}

  bool operator<(const StorageEdgeData& other) const {
    if(type != other.type) {
      return type < other.type;
    } else if(sourceNodeId != other.sourceNodeId) {
      return sourceNodeId < other.sourceNodeId;
    } else {
      return targetNodeId < other.targetNodeId;
    }
  }

  int type = 0;
  Id sourceNodeId = 0;
  Id targetNodeId = 0;
};

struct StorageEdge : public StorageEdgeData {
  StorageEdge() : StorageEdgeData() {}

  StorageEdge(Id id_, const StorageEdgeData& data) : StorageEdgeData(data), id(id_) {}

  StorageEdge(Id id_, int type_, Id sourceNodeId_, Id targetNodeId_)
      : StorageEdgeData(type_, sourceNodeId_, targetNodeId_), id(id_) {}

  Id id = 0;
};
