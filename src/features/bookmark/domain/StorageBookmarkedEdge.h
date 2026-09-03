#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageBookmarkedEdgeData {
  StorageBookmarkedEdgeData()
      : bookmarkId(0), serializedSourceNodeName(L""), serializedTargetNodeName(L""), edgeType(0), sourceNodeActive(false) {}

  StorageBookmarkedEdgeData(Id bookmarkId_,
                            const std::wstring& serializedSourceNodeName_,
                            const std::wstring& serializedTargetNodeName_,
                            int edgeType_,
                            bool sourceNodeActive_)
      : bookmarkId(bookmarkId_)
      , serializedSourceNodeName(serializedSourceNodeName_)
      , serializedTargetNodeName(serializedTargetNodeName_)
      , edgeType(edgeType_)
      , sourceNodeActive(sourceNodeActive_) {}

  Id bookmarkId;
  std::wstring serializedSourceNodeName;
  std::wstring serializedTargetNodeName;
  int edgeType;
  bool sourceNodeActive;
};

struct StorageBookmarkedEdge final : public StorageBookmarkedEdgeData {
  StorageBookmarkedEdge() : StorageBookmarkedEdgeData(), id(0) {}

  StorageBookmarkedEdge(Id id_, const StorageBookmarkedEdgeData& data) : StorageBookmarkedEdgeData(data), id(id_) {}

  StorageBookmarkedEdge(Id id_,
                        Id bookmarkId_,
                        const std::wstring& serializedSourceNodeName_,
                        const std::wstring& serializedTargetNodeName_,
                        int edgeType_,
                        bool sourceNodeActive_)
      : StorageBookmarkedEdgeData(bookmarkId_, serializedSourceNodeName_, serializedTargetNodeName_, edgeType_, sourceNodeActive_)
      , id(id_) {}

  Id id;
};