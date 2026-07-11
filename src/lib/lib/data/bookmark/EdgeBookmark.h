#pragma once
#include "Bookmark.h"

class EdgeBookmark final : public Bookmark {
public:
  EdgeBookmark(
      Id id, const std::wstring& name, const std::wstring& comment, const TimeStamp& timeStamp, const BookmarkCategory& category);

  ~EdgeBookmark() override;

  void addEdgeId(Id edgeId);
  void setEdgeIds(const std::vector<Id>& edgesIds);
  [[nodiscard]] std::vector<Id> getEdgeIds() const;

  void setActiveNodeId(Id activeNodeId);
  [[nodiscard]] Id getActiveNodeId() const;

private:
  std::vector<Id> mEdgeIds;
  Id mActiveNodeId;
};
