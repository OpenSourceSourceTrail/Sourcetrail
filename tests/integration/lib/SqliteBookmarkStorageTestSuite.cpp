#include <gtest/gtest.h>

#include "FileSystem.h"
#include "SqliteBookmarkStorage.h"

namespace {

TEST(SqliteBookmarkStorage, addBookmarks) {
  const FilePath databasePath(L"data/SQLiteTestSuite/bookmarkTest.sqlite");
  const size_t bookmarkCount = 4;
  int result = -1;
  {
    FileSystem::remove(databasePath);
    SqliteBookmarkStorage storage(databasePath);
    storage.setup();

    for(size_t i = 0; i < bookmarkCount; i++) {
      const Id categoryId = storage.addBookmarkCategory(StorageBookmarkCategoryData(L"test category")).id;
      storage.addBookmark(StorageBookmarkData(L"test bookmark", L"test comment", TimeStamp::now().toString(), categoryId));
    }

    result = static_cast<int>(storage.getAllBookmarks().size());
  }

  FileSystem::remove(databasePath);

  EXPECT_EQ(result, bookmarkCount);
}

TEST(SqliteBookmarkStorage, addBookmarkedNode) {
  const FilePath databasePath(L"data/SQLiteTestSuite/bookmarkTest.sqlite");
  const size_t bookmarkCount = 4;
  int result = -1;
  {
    FileSystem::remove(databasePath);
    SqliteBookmarkStorage storage(databasePath);
    storage.setup();

    const Id categoryId = storage.addBookmarkCategory(StorageBookmarkCategoryData(L"test category")).id;
    const Id bookmarkId =
        storage.addBookmark(StorageBookmarkData(L"test bookmark", L"test comment", TimeStamp::now().toString(), categoryId)).id;

    for(size_t i = 0; i < bookmarkCount; i++) {
      storage.addBookmarkedNode(StorageBookmarkedNodeData(bookmarkId, L"test name"));
    }

    result = static_cast<int>(storage.getAllBookmarkedNodes().size());
  }

  FileSystem::remove(databasePath);

  EXPECT_EQ(result, bookmarkCount);
}

TEST(SqliteBookmarkStorage, removeBookmarkAlsoRemovesBookmarkedNode) {
  FilePath databasePath(L"data/SQLiteTestSuite/bookmarkTest.sqlite");
  int result = -1;
  {
    FileSystem::remove(databasePath);
    SqliteBookmarkStorage storage(databasePath);
    storage.setup();

    const Id categoryId = storage.addBookmarkCategory(StorageBookmarkCategoryData(L"test category")).id;
    const Id bookmarkId =
        storage.addBookmark(StorageBookmarkData(L"test bookmark", L"test comment", TimeStamp::now().toString(), categoryId)).id;
    storage.addBookmarkedNode(StorageBookmarkedNodeData(bookmarkId, L"test name"));

    storage.removeBookmark(bookmarkId);

    result = static_cast<int>(storage.getAllBookmarkedNodes().size());
  }

  FileSystem::remove(databasePath);

  EXPECT_EQ(0, result);
  ;
}

TEST(SqliteBookmarkStorage, editNodeBookmark) {
  const FilePath databasePath(L"data/SQLiteTestSuite/bookmarkTest.sqlite");

  const std::wstring updatedName = L"updated name";
  const std::wstring updatedComment = L"updated comment";

  StorageBookmark storageBookmark;
  {
    FileSystem::remove(databasePath);
    SqliteBookmarkStorage storage(databasePath);
    storage.setup();

    const Id categoryId = storage.addBookmarkCategory(StorageBookmarkCategoryData(L"test category")).id;
    const Id bookmarkId =
        storage.addBookmark(StorageBookmarkData(L"test bookmark", L"test comment", TimeStamp::now().toString(), categoryId)).id;
    storage.addBookmarkedNode(StorageBookmarkedNodeData(bookmarkId, L"test name"));

    storage.updateBookmark(bookmarkId, updatedName, updatedComment, categoryId);

    storageBookmark = storage.getAllBookmarks().front();
  }

  FileSystem::remove(databasePath);

  EXPECT_EQ(updatedName, storageBookmark.name);
  EXPECT_EQ(updatedComment, storageBookmark.comment);
}

class BaseSqliteBookmarkStorage : public ::testing::Test {
protected:
  void SetUp() override {
    // Use in-memory SQLite database for testing
    mStorage = std::make_unique<SqliteBookmarkStorage>();
    mStorage->setup();
  }

  void TearDown() override {
    mStorage.reset();
  }

  std::unique_ptr<SqliteBookmarkStorage> mStorage;
};

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkCategory_Success) {
  // Given
  StorageBookmarkCategoryData categoryData;
  categoryData.name = L"Test Category";

  // When
  const StorageBookmarkCategory result = mStorage->addBookmarkCategory(categoryData);

  // Then
  EXPECT_GT(result.id, 0);    // ID should be positive
  EXPECT_EQ(result.name, categoryData.name);

  // Verify category was actually stored
  const auto categories = mStorage->getAllBookmarkCategories();
  ASSERT_EQ(categories.size(), 1);
  EXPECT_EQ(categories[0].id, result.id);
  EXPECT_EQ(categories[0].name, categoryData.name);
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkCategory_EmptyName) {
  // Given
  StorageBookmarkCategoryData categoryData;
  categoryData.name = L"";    // Empty name

  // When
  const StorageBookmarkCategory result = mStorage->addBookmarkCategory(categoryData);

  // Then
  EXPECT_GT(result.id, 0);    // Should still create with empty name
  EXPECT_EQ(result.name, categoryData.name);
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkCategory_UnicodeCharacters) {
  // Given
  StorageBookmarkCategoryData categoryData;
  categoryData.name = L"テスト카테고리";    // Mixed Unicode characters

  // When
  const StorageBookmarkCategory result = mStorage->addBookmarkCategory(categoryData);

  // Then
  EXPECT_GT(result.id, 0);
  EXPECT_EQ(result.name, categoryData.name);

  // Verify Unicode was preserved
  const auto categories = mStorage->getAllBookmarkCategories();
  ASSERT_EQ(categories.size(), 1);
  EXPECT_EQ(categories[0].name, categoryData.name);
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkedNode_ValidData_Success) {
  // Setup
  const StorageBookmarkCategoryData categoryData{L"TestCategory"};
  const auto category = mStorage->addBookmarkCategory(categoryData);

  const StorageBookmarkData bookmarkData{L"TestBookmark", L"TestComment", "2024-03-20", category.id};
  const auto bookmark = mStorage->addBookmark(bookmarkData);

  const StorageBookmarkedNodeData nodeData{bookmark.id, L"TestNode"};

  // Execute
  const auto result = mStorage->addBookmarkedNode(nodeData);

  // Verify
  ASSERT_NE(result.id, 0);
  EXPECT_EQ(result.bookmarkId, nodeData.bookmarkId);
  EXPECT_EQ(result.serializedNodeName, nodeData.serializedNodeName);

  // Verify persistence
  const auto allNodes = mStorage->getAllBookmarkedNodes();
  ASSERT_EQ(allNodes.size(), 1);
  EXPECT_EQ(allNodes[0].id, result.id);
  EXPECT_EQ(allNodes[0].bookmarkId, nodeData.bookmarkId);
  EXPECT_EQ(allNodes[0].serializedNodeName, nodeData.serializedNodeName);
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkedNode_EmptyNodeName_Success) {
  // Setup
  const StorageBookmarkCategoryData categoryData{L"TestCategory"};
  const auto category = mStorage->addBookmarkCategory(categoryData);

  const StorageBookmarkData bookmarkData{L"TestBookmark", L"TestComment", "2024-03-20", category.id};
  const auto bookmark = mStorage->addBookmark(bookmarkData);

  const StorageBookmarkedNodeData nodeData{
      bookmark.id,
      L""    // Empty node name
  };

  // Execute
  const auto result = mStorage->addBookmarkedNode(nodeData);

  // Verify
  ASSERT_NE(result.id, 0);
  EXPECT_EQ(result.serializedNodeName, L"");
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkedNode_SpecialCharacters_Success) {
  // Setup
  const StorageBookmarkCategoryData categoryData{L"TestCategory"};
  const auto category = mStorage->addBookmarkCategory(categoryData);

  const StorageBookmarkData bookmarkData{L"TestBookmark", L"TestComment", "2024-03-20", category.id};
  const auto bookmark = mStorage->addBookmark(bookmarkData);

  const StorageBookmarkedNodeData nodeData{bookmark.id, L"Test©Node™with★Special☆Characters"};

  // Execute
  const auto result = mStorage->addBookmarkedNode(nodeData);

  // Verify
  ASSERT_NE(result.id, 0);
  EXPECT_EQ(result.serializedNodeName, nodeData.serializedNodeName);

  // Verify persistence and correct UTF-8 handling
  const auto allNodes = mStorage->getAllBookmarkedNodes();
  ASSERT_EQ(allNodes.size(), 1);
  EXPECT_EQ(allNodes[0].serializedNodeName, nodeData.serializedNodeName);
}

TEST_F(BaseSqliteBookmarkStorage, AddBookmarkedEdge_Success) {
  // Setup
  StorageBookmarkCategoryData categoryData;
  categoryData.name = L"TestCategory";
  const auto category = mStorage->addBookmarkCategory(categoryData);

  StorageBookmarkData bookmarkData;
  bookmarkData.name = L"TestBookmark";
  bookmarkData.comment = L"Test Comment";
  bookmarkData.timestamp = "2024-03-20";
  bookmarkData.categoryId = category.id;
  const auto bookmark = mStorage->addBookmark(bookmarkData);

  // Test data
  StorageBookmarkedEdgeData edgeData;
  edgeData.bookmarkId = bookmark.id;
  edgeData.serializedSourceNodeName = L"SourceNode";
  edgeData.serializedTargetNodeName = L"TargetNode";
  edgeData.edgeType = 1;
  edgeData.sourceNodeActive = true;

  // Execute
  const auto result = mStorage->addBookmarkedEdge(edgeData);

  // Verify
  EXPECT_GT(result.id, 0);
  EXPECT_EQ(result.bookmarkId, edgeData.bookmarkId);
  EXPECT_EQ(result.serializedSourceNodeName, edgeData.serializedSourceNodeName);
  EXPECT_EQ(result.serializedTargetNodeName, edgeData.serializedTargetNodeName);
  EXPECT_EQ(result.edgeType, edgeData.edgeType);
  EXPECT_EQ(result.sourceNodeActive, edgeData.sourceNodeActive);

  // Verify the edge was actually stored in the database
  const auto allEdges = mStorage->getAllBookmarkedEdges();
  ASSERT_EQ(allEdges.size(), 1);
  EXPECT_EQ(allEdges[0].id, result.id);
  EXPECT_EQ(allEdges[0].bookmarkId, edgeData.bookmarkId);
  EXPECT_EQ(allEdges[0].serializedSourceNodeName, edgeData.serializedSourceNodeName);
  EXPECT_EQ(allEdges[0].serializedTargetNodeName, edgeData.serializedTargetNodeName);
  EXPECT_EQ(allEdges[0].edgeType, edgeData.edgeType);
  EXPECT_EQ(allEdges[0].sourceNodeActive, edgeData.sourceNodeActive);
}

TEST_F(BaseSqliteBookmarkStorage, DoGetFirstReturnsFirstItemWhenResultsExist) {
  // Arrange
  StorageBookmarkCategoryData categoryData;
  categoryData.name = L"Test Category";
  mStorage->addBookmarkCategory(categoryData);

  // Act
  const StorageBookmarkCategory result = mStorage->getBookmarkCategoryByName(L"Test Category");

  // Assert
  EXPECT_GT(result.id, 0);
  EXPECT_EQ(result.name, L"Test Category");
}

TEST_F(BaseSqliteBookmarkStorage, DoGetFirstReturnsEmptyItemWhenNoResultsExist) {
  // Act
  const StorageBookmarkCategory result = mStorage->getBookmarkCategoryByName(L"NonExistentCategory");

  // Assert
  EXPECT_EQ(result.id, 0);
  EXPECT_TRUE(result.name.empty());
}

TEST_F(BaseSqliteBookmarkStorage, MigrateIfNecessary_EmptyStore) {
  EXPECT_NO_THROW(mStorage->migrateIfNecessary());
}

class TestSqliteBookmarkStorage : public BaseSqliteBookmarkStorage {
protected:
  void SetUp() override {
    BaseSqliteBookmarkStorage::SetUp();

    // Setup common test category
    StorageBookmarkCategoryData category_data;
    category_data.name = L"Test Category";
    mTestCategory = mStorage->addBookmarkCategory(category_data);
  }

  [[nodiscard]] StorageBookmarkData CreateValidBookmarkData() const {
    return {L"Test Bookmark", L"Test Comment", "2024-03-20 12:00:00", mTestCategory.id};
  }

  StorageBookmarkCategory mTestCategory;
};

TEST_F(TestSqliteBookmarkStorage, AddBookmark_ValidData_CreatesBookmark) {
  // Given
  const StorageBookmarkData bookmarkData = CreateValidBookmarkData();

  // When
  const StorageBookmark result = mStorage->addBookmark(bookmarkData);

  // Then
  EXPECT_NE(result.id, 0);
  EXPECT_EQ(result.name, bookmarkData.name);
  EXPECT_EQ(result.comment, bookmarkData.comment);
  EXPECT_EQ(result.timestamp, bookmarkData.timestamp);
  EXPECT_EQ(result.categoryId, bookmarkData.categoryId);

  // Verify persistence
  const std::vector<StorageBookmark> storedBookmarks = mStorage->getAllBookmarks();
  ASSERT_EQ(storedBookmarks.size(), 1);
  EXPECT_EQ(storedBookmarks[0].id, result.id);
  EXPECT_EQ(storedBookmarks[0].name, bookmarkData.name);
}

TEST_F(TestSqliteBookmarkStorage, AddBookmark_EmptyFields_CreatesBookmark) {
  // Given
  StorageBookmarkData bookmarkData = CreateValidBookmarkData();
  bookmarkData.name = L"";
  bookmarkData.comment = L"";

  // When
  const StorageBookmark result = mStorage->addBookmark(bookmarkData);

  // Then
  EXPECT_NE(result.id, 0);
  EXPECT_TRUE(result.name.empty());
  EXPECT_TRUE(result.comment.empty());
  EXPECT_EQ(result.timestamp, bookmarkData.timestamp);
  EXPECT_EQ(result.categoryId, bookmarkData.categoryId);
}

TEST_F(TestSqliteBookmarkStorage, AddBookmark_DuplicateData_CreatesSeparateBookmarks) {
  // Given
  const StorageBookmarkData bookmarkData = CreateValidBookmarkData();

  // When
  const StorageBookmark first_result = mStorage->addBookmark(bookmarkData);
  const StorageBookmark second_result = mStorage->addBookmark(bookmarkData);

  // Then
  EXPECT_NE(first_result.id, 0);
  EXPECT_NE(second_result.id, 0);
  EXPECT_NE(first_result.id, second_result.id);

  // Verify both bookmarks were stored
  std::vector<StorageBookmark> stored_bookmarks = mStorage->getAllBookmarks();
  ASSERT_EQ(stored_bookmarks.size(), 2);
  EXPECT_EQ(stored_bookmarks[0].id, first_result.id);
  EXPECT_EQ(stored_bookmarks[1].id, second_result.id);
}

}    // namespace
