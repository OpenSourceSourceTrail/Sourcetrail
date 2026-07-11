#pragma once
// STL
#include <limits>
// internal
#include "GlobalId.hpp"

struct StorageSourceLocationData {
  static constexpr size_t Invalid = std::numeric_limits<size_t>::max();

  StorageSourceLocationData() = default;

  StorageSourceLocationData(Id fileNodeId_, size_t startLine_, size_t startCol_, size_t endLine_, size_t endCol_, int type_)
      : fileNodeId(fileNodeId_), startLine(startLine_), startCol(startCol_), endLine(endLine_), endCol(endCol_), type(type_) {}

  bool operator<(const StorageSourceLocationData& other) const {
    if(fileNodeId != other.fileNodeId) {
      return fileNodeId < other.fileNodeId;
    } else if(startLine != other.startLine) {
      return startLine < other.startLine;
    } else if(startCol != other.startCol) {
      return startCol < other.startCol;
    } else if(endLine != other.endLine) {
      return endLine < other.endLine;
    } else if(endCol != other.endCol) {
      return endCol < other.endCol;
    } else {
      return type < other.type;
    }
  }

  Id fileNodeId = 0;
  size_t startLine = Invalid;
  size_t startCol = Invalid;
  size_t endLine = Invalid;
  size_t endCol = Invalid;
  int type = 0;
};

struct StorageSourceLocation : public StorageSourceLocationData {
  StorageSourceLocation() : StorageSourceLocationData() {}

  StorageSourceLocation(Id id_, const StorageSourceLocationData& data) : StorageSourceLocationData(data), id(id_) {}

  StorageSourceLocation(Id id_, Id fileNodeId_, size_t startLine_, size_t startCol_, size_t endLine_, size_t endCol_, int type_)
      : StorageSourceLocationData(fileNodeId_, startLine_, startCol_, endLine_, endCol_, type_), id(id_) {}

  Id id = 0;
};
