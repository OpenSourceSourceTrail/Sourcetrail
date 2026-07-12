#include "EdgeBookmark.h"

EdgeBookmark::EdgeBookmark(
    Id id, const std::wstring& name, const std::wstring& comment, const TimeStamp& timeStamp, const BookmarkCategory& category)
    : Bookmark(id, name, comment, timeStamp, category) {}

EdgeBookmark::~EdgeBookmark() = default;

void EdgeBookmark::addEdgeId(Id edgeId) {
  mEdgeIds.push_back(edgeId);
}

void EdgeBookmark::setEdgeIds(const std::vector<Id>& edgesIds) {
  mEdgeIds = edgesIds;
}

std::vector<Id> EdgeBookmark::getEdgeIds() const {
  return mEdgeIds;
}

void EdgeBookmark::setActiveNodeId(Id activeNodeId) {
  mActiveNodeId = activeNodeId;
}

Id EdgeBookmark::getActiveNodeId() const {
  return mActiveNodeId;
}
