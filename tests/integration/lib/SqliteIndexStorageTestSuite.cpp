#include <gtest/gtest.h>

#include <CppSQLite3.h>

#include "FileSystem.h"
#ifndef _WIN32
#  define private public    // NOLINT(clang-diagnostic-keyword-macro)
#endif
#include "data/storage/sqlite/SqliteIndexStorage.h"
#ifndef _WIN32
#  undef private
#endif

namespace {

TEST(SqliteIndexStorage, addsNodeSuccessfully) {
  const FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int nodeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    storage.addNode(StorageNodeData(0, L"a"));
    storage.commitTransaction();
    nodeCount = storage.getNodeCount();
  }
  std::ignore = FileSystem::remove(databasePath);

  EXPECT_TRUE(1 == nodeCount);
}

TEST(SqliteIndexStorage, removesNodeSuccessfully) {
  const FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int nodeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    const Id nodeId = storage.addNode(StorageNodeData(0, L"a"));
    storage.removeElement(nodeId);
    storage.commitTransaction();
    nodeCount = storage.getNodeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(0 == nodeCount);
}

TEST(SqliteIndexStorage, addsEdgeSuccessfully) {
  const FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int edgeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    const Id sourceNodeId = storage.addNode(StorageNodeData(0, L"a"));
    const Id targetNodeId = storage.addNode(StorageNodeData(0, L"b"));
    storage.addEdge(StorageEdgeData(0, sourceNodeId, targetNodeId));
    storage.commitTransaction();
    edgeCount = storage.getEdgeCount();
  }
  FileSystem::remove(databasePath);

  EXPECT_TRUE(1 == edgeCount);
}

TEST(SqliteIndexStorage, removesEdgeSuccessfully) {
  const FilePath databasePath(L"data/SQLiteTestSuite/test.sqlite");
  int edgeCount = -1;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.beginTransaction();
    const Id sourceNodeId = storage.addNode(StorageNodeData(0, L"a"));
    const Id targetNodeId = storage.addNode(StorageNodeData(0, L"b"));
    const Id edgeId = storage.addEdge(StorageEdgeData(0, sourceNodeId, targetNodeId));
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
  constexpr auto Node1Type = 42;
  // Given:
  StorageNode node;
  node.id = 1;
  node.type = Node1Type;
  ASSERT_EQ(1, mStorage->addNode(node));

  // When:
  const auto result = mStorage->doGetFirst<StorageNode>("WHERE id = 1");

  // Then:
  EXPECT_EQ(result.id, 1);
  EXPECT_EQ(result.type, Node1Type);
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_ReturnsEmptyOnNoMatch) {
  const auto result = mStorage->doGetFirst<StorageNode>("WHERE id = 999");

  EXPECT_EQ(result.id, 0);    // Assuming default constructed StorageNode has id = 0
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_HandlesInvalidQuery) {
  EXPECT_THROW(mStorage->doGetFirst<StorageNode>("INVALID SQL"), CppSQLite3Exception);
}

TEST_F(SqliteIndexStorageTest, DoGetFirst_ReturnsOnlyFirstWhenMultipleExist) {
  constexpr auto Node1Type = 42;
  constexpr auto Node2Type = 43;
  // Add multiple nodes
  StorageNode node1;
  node1.id = 1;
  node1.type = Node1Type;
  mStorage->addNode(node1);

  StorageNode node2;
  node2.id = 2;
  node2.type = Node2Type;
  mStorage->addNode(node2);

  const auto result = mStorage->doGetFirst<StorageNode>("");

  EXPECT_EQ(result.id, 1);
  EXPECT_EQ(result.type, Node1Type);
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

  const Id id1 = mStorage->addNode(node1);
  mStorage->addNode(node2);

  // Test with specific query
  std::vector<StorageNode> results = mStorage->doGetAll<StorageNode>("WHERE id == " + std::to_string(id1));

  // Verify
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].serializedName, L"node1");
}

TEST_F(SqliteIndexStorageTest, DoGetAll_EmptyResults_ReturnsEmptyVector) {
  // Test with query that should return no results
  const std::vector<StorageNode> results = mStorage->doGetAll<StorageNode>("WHERE id == -1");

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


#ifndef _WIN32
// The element table's rowid sequence backs the ids of nodes, edges, local symbols and source
// locations. Those are handed out from a single read of the sequence and the matching element rows
// go in as one batched insert afterwards, so a drift between the two would silently dangle every
// reference. These tests pin that down.

TEST_F(SqliteIndexStorageTest, AddNodes_IdsMatchTheElementRowsWrittenForThem) {
  // given
  std::vector<StorageNode> nodes;
  for(int i = 0; i < 10; i++) {
    nodes.emplace_back(0, StorageNodeData(i, L"node" + std::to_wstring(i)));
  }

  // when
  const Id firstId = mStorage->getNextElementId();
  const std::vector<Id> ids = mStorage->addNodes(nodes);

  // then
  ASSERT_EQ(nodes.size(), ids.size());
  for(size_t i = 0; i < ids.size(); i++) {
    EXPECT_EQ(ids[i], mStorage->getNodeById(ids[i]).id) << "node " << i << " is not stored under the id it was given";
  }
  EXPECT_EQ(firstId + nodes.size(), mStorage->getNextElementId()) << "element rows and node ids drifted apart";
}

TEST_F(SqliteIndexStorageTest, AddNodes_SecondBatchDoesNotReuseIdsFromTheFirst) {
  // given
  const std::vector<Id> first = mStorage->addNodes(
      {StorageNode(0, StorageNodeData(1, L"a")), StorageNode(0, StorageNodeData(1, L"b"))});

  // when
  const std::vector<Id> second = mStorage->addNodes(
      {StorageNode(0, StorageNodeData(1, L"c")), StorageNode(0, StorageNodeData(1, L"d"))});

  // then
  std::set<Id> unique(first.begin(), first.end());
  unique.insert(second.begin(), second.end());
  EXPECT_EQ(first.size() + second.size(), unique.size());
}

TEST_F(SqliteIndexStorageTest, AddNodes_DeduplicatedNodesConsumeNoElementRow) {
  // given
  const std::vector<Id> first = mStorage->addNodes({StorageNode(0, StorageNodeData(1, L"a"))});

  // when: the same serialized name comes back alongside a new one
  const std::vector<Id> second = mStorage->addNodes(
      {StorageNode(0, StorageNodeData(1, L"a")), StorageNode(0, StorageNodeData(1, L"b"))});

  // then: "a" resolves to the existing id and only "b" allocates
  EXPECT_EQ(first.front(), second.front());
  EXPECT_NE(second.front(), second.back());
  EXPECT_EQ(second.back() + 1, mStorage->getNextElementId()) << "the deduplicated node still consumed an element row";
}

TEST_F(SqliteIndexStorageTest, AddEdgesAndLocalSymbols_ShareTheElementSequenceWithoutCollidingWithNodes) {
  // given
  const std::vector<Id> nodeIds = mStorage->addNodes(
      {StorageNode(0, StorageNodeData(1, L"a")), StorageNode(0, StorageNodeData(1, L"b"))});

  // when
  const std::vector<Id> edgeIds = mStorage->addEdges({StorageEdge(0, StorageEdgeData(1, nodeIds[0], nodeIds[1]))});
  const std::vector<Id> symbolIds = mStorage->addLocalSymbols({StorageLocalSymbol(0, StorageLocalSymbolData(L"f<local>"))});

  // then
  std::set<Id> unique(nodeIds.begin(), nodeIds.end());
  unique.insert(edgeIds.begin(), edgeIds.end());
  unique.insert(symbolIds.begin(), symbolIds.end());
  EXPECT_EQ(nodeIds.size() + edgeIds.size() + symbolIds.size(), unique.size());
  EXPECT_EQ(edgeIds.front(), mStorage->getEdgeById(edgeIds.front()).id);
}

TEST_F(SqliteIndexStorageTest, InsertElementRows_ChunksBeyondASingleStatement) {
  // given: more rows than fit one chunked INSERT, to cover the multi-chunk path
  constexpr size_t RowCount = 1200;
  const Id firstId = mStorage->getNextElementId();

  // when
  const bool success = mStorage->insertElementRows(RowCount, firstId);

  // then
  EXPECT_TRUE(success);
  EXPECT_EQ(firstId + RowCount, mStorage->getNextElementId());
}

TEST_F(SqliteIndexStorageTest, GetEdgesBySourceOrTargetId_ReturnsEachEdgeOnce) {
  // given
  const std::vector<Id> nodeIds = mStorage->addNodes(
      {StorageNode(0, StorageNodeData(1, L"a")), StorageNode(0, StorageNodeData(1, L"b"))});
  const Id outgoing = mStorage->addEdge(StorageEdgeData(1, nodeIds[0], nodeIds[1]));
  const Id incoming = mStorage->addEdge(StorageEdgeData(1, nodeIds[1], nodeIds[0]));
  // a self-edge matches both halves of the query and must not be returned twice
  const Id self = mStorage->addEdge(StorageEdgeData(1, nodeIds[0], nodeIds[0]));

  // when
  const std::vector<StorageEdge> edges = mStorage->getEdgesBySourceOrTargetId(nodeIds[0]);

  // then
  std::set<Id> ids;
  for(const StorageEdge& edge : edges) {
    ids.insert(edge.id);
  }
  EXPECT_EQ(edges.size(), ids.size()) << "an edge was returned more than once";
  EXPECT_EQ(std::set<Id>({outgoing, incoming, self}), ids);
}
#endif


#ifndef _WIN32
namespace {
// Reads the schema through a second, independent connection, so the assertions below are about what
// actually landed in the file rather than about the storage object's own bookkeeping.
std::set<std::string> readIndexNames(const FilePath& databasePath) {
  CppSQLite3DB database;
  database.open(utility::encodeToUtf8(databasePath.wstr()).c_str());

  std::set<std::string> names;
  CppSQLite3Query query = database.execQuery("SELECT name FROM sqlite_master WHERE type = 'index';");
  while(!query.eof()) {
    names.emplace(query.getStringField(0, ""));
    query.nextRow();
  }
  return names;
}

// journal_mode=WAL is recorded in the database header, so another connection sees it. MEMORY is
// per-connection and leaves no trace, which is why the write phase is asserted negatively.
std::string readJournalMode(const FilePath& databasePath) {
  CppSQLite3DB database;
  database.open(utility::encodeToUtf8(databasePath.wstr()).c_str());

  CppSQLite3Query query = database.execQuery("PRAGMA journal_mode;");
  return query.eof() ? std::string() : utility::toLowerCase(std::string(query.getStringField(0, "")));
}
}    // namespace

// setMode() drops every index not tagged for the requested mode. Tagging a column the read path
// filters on for CLEAR or WRITE only means the GUI queries it with no index at all, which is a full
// table scan on every graph expansion. These names must stay present in READ mode.
TEST(SqliteIndexStorageModeTest, SetModeRead_KeepsTheIndexesTheReadPathQueriesThrough) {
  const FilePath databasePath(L"data/SQLiteTestSuite/mode_read.sqlite");
  std::ignore = FileSystem::remove(databasePath);

  std::set<std::string> names;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.setMode(SqliteIndexStorage::STORAGE_MODE_READ);
    names = readIndexNames(databasePath);
  }
  std::ignore = FileSystem::remove(databasePath);

  for(const char* required : {"edge_source_node_id_index",
                              "edge_target_node_id_index",
                              "node_serialized_name_index",
                              "source_location_file_node_id_index",
                              "file_path_index",
                              "occurrence_element_id_index",
                              "occurrence_source_location_id_index",
                              "element_component_element_id_index"}) {
    EXPECT_TRUE(names.count(required) == 1) << required << " is missing in STORAGE_MODE_READ";
  }
}

TEST(SqliteIndexStorageModeTest, SetModeWrite_DropsTheIndexesThatOnlySlowInsertsDown) {
  const FilePath databasePath(L"data/SQLiteTestSuite/mode_write.sqlite");
  std::ignore = FileSystem::remove(databasePath);

  std::set<std::string> names;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();
    storage.setMode(SqliteIndexStorage::STORAGE_MODE_WRITE);
    names = readIndexNames(databasePath);
  }
  std::ignore = FileSystem::remove(databasePath);

  // Bulk insert dedupes in memory, so these earn nothing during the write phase.
  EXPECT_TRUE(names.count("edge_source_node_id_index") == 0);
  EXPECT_TRUE(names.count("edge_target_node_id_index") == 0);
  EXPECT_TRUE(names.count("node_serialized_name_index") == 0);
  // addFile() looks a path up before every insert, so this one has to stay.
  EXPECT_TRUE(names.count("file_path_index") == 1);
}

TEST(SqliteIndexStorageModeTest, SetMode_SwitchesTheJournalToMatchThePhase) {
  const FilePath databasePath(L"data/SQLiteTestSuite/mode_journal.sqlite");
  std::ignore = FileSystem::remove(databasePath);

  std::string writeJournal;
  std::string readJournal;
  {
    SqliteIndexStorage storage(databasePath);
    storage.setup();

    storage.setMode(SqliteIndexStorage::STORAGE_MODE_WRITE);
    writeJournal = readJournalMode(databasePath);

    storage.setMode(SqliteIndexStorage::STORAGE_MODE_READ);
    readJournal = readJournalMode(databasePath);
  }
  std::ignore = FileSystem::remove(databasePath);

  EXPECT_NE("wal", writeJournal) << "the bulk-write phase should not be paying for WAL";
  EXPECT_EQ("wal", readJournal);
}
#endif

}    // namespace
