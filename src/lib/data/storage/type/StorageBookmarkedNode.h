#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageBookmarkedNodeData {
  StorageBookmarkedNodeData() : bookmarkId(0), serializedNodeName(L"") {}

  StorageBookmarkedNodeData(Id bookmarkId_, const std::wstring& serializedNodeName_)
      : bookmarkId(bookmarkId_), serializedNodeName(serializedNodeName_) {}

  Id bookmarkId;
  std::wstring serializedNodeName;
};

struct StorageBookmarkedNode final : public StorageBookmarkedNodeData {
  StorageBookmarkedNode() : StorageBookmarkedNodeData(), id(0) {}

  StorageBookmarkedNode(Id id_, const StorageBookmarkedNodeData& data) : StorageBookmarkedNodeData(data), id(id_) {}

  StorageBookmarkedNode(Id id_, Id bookmarkId_, const std::wstring& serializedNodeName_)
      : StorageBookmarkedNodeData(bookmarkId_, serializedNodeName_), id(id_) {}

  Id id;
};