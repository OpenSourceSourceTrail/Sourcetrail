#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ReadSqliteBookmarkStorage.hpp"
#include "SqliteFactory.hpp"
#include "StorageBookmark.h"
#include "StorageBookmarkCategory.h"
#include "StorageBookmarkedEdge.h"
#include "StorageBookmarkedNode.h"

namespace sqlite {

struct ReadSqliteBookmarkStorageFix : testing::Test {
  void SetUp() override {
    mStorage = SqliteFactory{}.createReadSqliteBookmarkStorage("data/SQLiteTestSuite/testBookmarks.sqlite");
    ASSERT_THAT(mStorage, testing::NotNull());
  }

  void TearDown() override {}

  std::unique_ptr<IReadSqliteBookmarkStorage> mStorage;
};

TEST_F(ReadSqliteBookmarkStorageFix, getAllBookmarks_GoodCase) {
  auto bookmarksResult = mStorage->getAllBookmarks();
  ASSERT_TRUE(bookmarksResult);
  const auto bookmarks = std::move(bookmarksResult.value());
  ASSERT_EQ(bookmarks.size(), 2);
  EXPECT_EQ(bookmarks[0].name, L"Application::createInstance");
  EXPECT_EQ(bookmarks[1].name, L"sqlite::ReadSqliteBookmarkStorage::~ReadSqliteBookmarkStorage");
}

TEST_F(ReadSqliteBookmarkStorageFix, getAllBookmarkCategories_GoodCase) {
  auto bookmarkCategoriesResult = mStorage->getAllBookmarkCategories();
  ASSERT_TRUE(bookmarkCategoriesResult);
  const auto bookmarkCategories = std::move(bookmarkCategoriesResult.value());
  ASSERT_EQ(bookmarkCategories.size(), 2);
  EXPECT_EQ(bookmarkCategories[0].name, L"Application");
  EXPECT_EQ(bookmarkCategories[1].name, L"ReadSqliteBookmarkStorage");
}

TEST_F(ReadSqliteBookmarkStorageFix, getBookmarkCategoryByName_GoodCase) {
  auto bookmarkCategoriesResult = mStorage->getBookmarkCategoryByName(L"Application");
  ASSERT_TRUE(bookmarkCategoriesResult);
  EXPECT_EQ(bookmarkCategoriesResult->name, L"Application");
}

TEST_F(ReadSqliteBookmarkStorageFix, getBookmarkCategoryByName_NoCategoryWithName) {
  auto bookmarkCategoriesResult = mStorage->getBookmarkCategoryByName(L"Missing");
  ASSERT_FALSE(bookmarkCategoriesResult);
  EXPECT_EQ(bookmarkCategoriesResult.error(), "No BookmarkCategory found by the following name: Missing");
}

TEST_F(ReadSqliteBookmarkStorageFix, getAllBookmarkedNodes_GoodCase) {
  auto bookmarkedNodesResult = mStorage->getAllBookmarkedNodes();
  ASSERT_TRUE(bookmarkedNodesResult);
  const auto bookmarkedNodes = std::move(bookmarkedNodesResult.value());
  ASSERT_EQ(bookmarkedNodes.size(), 2);
}

TEST_F(ReadSqliteBookmarkStorageFix, getAllBookmarkedEdges_GoodCase) {
  auto bookmarkedNodesResult = mStorage->getAllBookmarkedEdges();
  ASSERT_TRUE(bookmarkedNodesResult);
  const auto bookmarkedNodes = std::move(bookmarkedNodesResult.value());
  EXPECT_THAT(bookmarkedNodes, testing::IsEmpty());
}

}    // namespace sqlite
