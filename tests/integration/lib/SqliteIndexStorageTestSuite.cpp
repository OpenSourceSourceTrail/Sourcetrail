#include <gtest/gtest.h>

#include "FileSystem.h"
#ifndef _WIN32
#define private public
#endif
#include "SqliteIndexStorage.h"
#ifndef _WIN32
#undef private
#endif

namespace {

TEST(SqliteIndexStorage, addsNodeSuccessfully) {
  FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int nodeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    storage.addNode(StorageNodeData(0, L"a"));
    storage.commitTransaction();
    nodeCount = storage.getNodeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(1 == nodeCount);
}

TEST(SqliteIndexStorage, removesNodeSuccessfully) {
  FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int nodeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    Id nodeId = storage.addNode(StorageNodeData(0, L"a"));
    storage.removeElement(nodeId);
    storage.commitTransaction();
    nodeCount = storage.getNodeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(0 == nodeCount);
}

TEST(SqliteIndexStorage, addsEdgeSuccessfully) {
  FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int edgeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    Id sourceNodeId = storage.addNode(StorageNodeData(0, L"a"));
    Id targetNodeId = storage.addNode(StorageNodeData(0, L"b"));
    storage.addEdge(StorageEdgeData(0, sourceNodeId, targetNodeId));
    storage.commitTransaction();
    edgeCount = storage.getEdgeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(1 == edgeCount);
}

TEST(SqliteIndexStorage, removesEdgeSuccessfully) {
  FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int edgeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    Id sourceNodeId = storage.addNode(StorageNodeData(0, L"a"));
    Id targetNodeId = storage.addNode(StorageNodeData(0, L"b"));
    Id edgeId = storage.addEdge(StorageEdgeData(0, sourceNodeId, targetNodeId));
    storage.removeElement(edgeId);
    storage.commitTransaction();
    edgeCount = storage.getEdgeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(0 == edgeCount);
}

class SqliteIndexStorageTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup temporary database for testing
    mStorage = std::make_unique<SqliteIndexStorage>();
    mStorage->setup();
  }

  std::unique_ptr<SqliteIndexStorage> mStorage;
};

#ifndef _WIN32
TEST_F(SqliteIndexStorageTest, DoGetFirst_ReturnsFirstElement) {
  // Given:
  StorageNode node;
  node.id = 1;
  node.type = 42;
  ASSERT_EQ(1, mStorage->addNode(node));

  // When:
  const auto result = mStorage->doGetFirst<StorageNode>("WHERE id = 1");

  // Then:
  EXPECT_EQ(result.id, 1);
  EXPECT_EQ(result.type, 42);
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_ReturnsEmptyOnNoMatch) {
  const auto result = mStorage->doGetFirst<StorageNode>("WHERE id = 999");

  EXPECT_EQ(result.id, 0);    // Assuming default constructed StorageNode has id = 0
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_HandlesInvalidQuery) {
  EXPECT_THROW(mStorage->doGetFirst<StorageNode>("INVALID SQL"), CppSQLite3Exception);
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_ReturnsOnlyFirstWhenMultipleExist) {
  // Add multiple nodes
  StorageNode node1;
  node1.id = 1;
  node1.type = 42;
  mStorage->addNode(node1);

  StorageNode node2;
  node2.id = 2;
  node2.type = 43;
  mStorage->addNode(node2);

  const auto result = mStorage->doGetFirst<StorageNode>("");

  EXPECT_EQ(result.id, 1);
  EXPECT_EQ(result.type, 42);
}
#endif

TEST_F(SqliteIndexStorageTest, DoGetAll_EmptyQuery_ReturnsAllNodes) {
  // Setup test data
  StorageNodeData node1;
  node1.serializedName = L"node1";
  node1.type = 1;

  StorageNodeData node2;
  node2.serializedName = L"node2";
  node2.type = 1;

  mStorage->addNode(node1);
  mStorage->addNode(node2);

  // Test
  std::vector<StorageNode> results = mStorage->getAll<StorageNode>();

  // Verify
  ASSERT_EQ(results.size(), 2);
  EXPECT_EQ(results[0].serializedName, L"node1");
  EXPECT_EQ(results[1].serializedName, L"node2");
}

#ifndef _WIN32
TEST_F(SqliteIndexStorageTest, DoGetAll_WithQuery_ReturnsFilteredNodes) {
  // Setup test data
  StorageNodeData node1;
  node1.serializedName = L"node1";
  node1.type = 1;

  StorageNodeData node2;
  node2.serializedName = L"node2";
  node2.type = 2;

  Id id1 = mStorage->addNode(node1);
  mStorage->addNode(node2);

  // Test with specific query
  std::vector<StorageNode> results = mStorage->doGetAll<StorageNode>("WHERE id == " + std::to_string(id1));

  // Verify
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].serializedName, L"node1");
}

TEST_F(SqliteIndexStorageTest, DoGetAll_EmptyResults_ReturnsEmptyVector) {
  // Test with query that should return no results
  std::vector<StorageNode> results = mStorage->doGetAll<StorageNode>("WHERE id == -1");

  // Verify
  EXPECT_TRUE(results.empty());
}
#endif

TEST_F(SqliteIndexStorageTest, GetProjectSettingsText_EmptyByDefault) {
  // When a new storage is created, project settings should be empty
  EXPECT_EQ(mStorage->getProjectSettingsText(), "");
}

TEST_F(SqliteIndexStorageTest, GetProjectSettingsText_AfterSetting) {
  // Given
  const std::string expectedText = "test project settings";

  // When
  mStorage->setProjectSettingsText(expectedText);

  // Then
  EXPECT_EQ(mStorage->getProjectSettingsText(), expectedText);
}

TEST_F(SqliteIndexStorageTest, GetProjectSettingsText_WithSpecialCharacters) {
  // Given
  const std::string expectedText = "test\nproject\tsettings\r\nwith\"special'chars";

  // When
  mStorage->setProjectSettingsText(expectedText);

  // Then
  EXPECT_EQ(mStorage->getProjectSettingsText(), expectedText);
}

TEST_F(SqliteIndexStorageTest, GetProjectSettingsText_ReadOnlyMode) {
  // Given
  const std::string expectedText = "test settings";
  mStorage->setProjectSettingsText(expectedText);

  // When
  mStorage->setMode(SqliteIndexStorage::STORAGE_MODE_READ);

  // Then
  EXPECT_EQ(mStorage->getProjectSettingsText(), expectedText);
}

TEST_F(SqliteIndexStorageTest, SetProjectSettingsText_StoresTextInDatabase) {
  // given
  const std::string expectedSettings = "test project settings";

  // when
  mStorage->setProjectSettingsText(expectedSettings);

  // then
  EXPECT_EQ(mStorage->getProjectSettingsText(), expectedSettings);
}

TEST_F(SqliteIndexStorageTest, SetProjectSettingsText_OverwritesExistingSettings) {
  // given
  const std::string initialSettings = "initial settings";
  const std::string updatedSettings = "updated settings";
  mStorage->setProjectSettingsText(initialSettings);

  // when
  mStorage->setProjectSettingsText(updatedSettings);

  // then
  EXPECT_EQ(mStorage->getProjectSettingsText(), updatedSettings);
}

TEST_F(SqliteIndexStorageTest, SetProjectSettingsText_HandlesEmptyString) {
  // given
  const std::string emptySettings;

  // when
  mStorage->setProjectSettingsText(emptySettings);

  // then
  EXPECT_EQ(mStorage->getProjectSettingsText(), emptySettings);
}

TEST_F(SqliteIndexStorageTest, SetProjectSettingsText_HandlesSpecialCharacters) {
  // given
  const std::string settingsWithSpecialChars = "test\nproject\tsettings\r\nwith\"quotes'and\\slashes";

  // when
  mStorage->setProjectSettingsText(settingsWithSpecialChars);

  // then
  EXPECT_EQ(mStorage->getProjectSettingsText(), settingsWithSpecialChars);
}

#ifndef _WIN32
TEST_F(SqliteIndexStorageTest, SetMode_ClearsAllTemporaryIndices) {
  // given
  mStorage->addNode(StorageNodeData(1, L"test"));    // Populate temp indices
  mStorage->addEdge(StorageEdgeData(1, 1, 2));
  mStorage->addLocalSymbol(StorageLocalSymbolData(L"test<local>"));

  // when
  mStorage->setMode(SqliteIndexStorage::STORAGE_MODE_CLEAR);

  // then
  EXPECT_TRUE(mStorage->m_tempNodeNameIndex.empty());
  EXPECT_TRUE(mStorage->m_tempWNodeNameIndex.empty());
  EXPECT_TRUE(mStorage->m_tempNodeTypes.empty());
  EXPECT_TRUE(mStorage->m_tempEdgeIndex.empty());
  EXPECT_TRUE(mStorage->m_tempLocalSymbolIndex.empty());
  EXPECT_TRUE(mStorage->m_tempSourceLocationIndices.empty());
}
#endif

}    // namespace
