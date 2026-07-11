#pragma once
#include <gmock/gmock.h>

#include "BookmarkView.h"

class ViewLayout;

struct MockedBookmarkView : BookmarkView {
  MockedBookmarkView(ViewLayout* viewLayout) : BookmarkView(viewLayout) {}
  MOCK_METHOD(void, displayBookmarks, (const std::vector<std::shared_ptr<Bookmark>>&), (override));
  MOCK_METHOD(void, displayBookmarkEditor, (std::shared_ptr<Bookmark>, const std::vector<BookmarkCategory>&), (override));
  MOCK_METHOD(void, displayBookmarkCreator, (const std::vector<std::wstring>&, const std::vector<BookmarkCategory>&, Id), (override));
  MOCK_METHOD(bool, bookmarkBrowserIsVisible, (), (const, override));

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
};
