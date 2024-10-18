#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IntermediateStorage.h"
#include "LocationType.h"

namespace {

using testing::IsEmpty;

bool operator==(const StorageNodeData& a, const StorageNodeData& b) {
  return a.type == b.type && a.serializedName == b.serializedName;
}

TEST(IntermediateStorageFix, clear) {
  // Given:
  IntermediateStorage storage;
  storage.setStorageFiles({
      StorageFile{3, L"main.cpp", L"cpp", "now", true, true},
      StorageFile{5, L"application.cpp", L"cpp", "now", false, false},
  });
  storage.setStorageSourceLocations({
      StorageSourceLocation{0, 3, 0, 0, 1, 1, locationTypeToInt(LOCATION_ERROR)},
      StorageSourceLocation{1, 4, 0, 0, 1, 1, locationTypeToInt(LOCATION_ERROR)},
      StorageSourceLocation{2, 5, 0, 0, 1, 1, locationTypeToInt(LOCATION_SIGNATURE)},
  });
  storage.setStorageOccurrences({{}});
  storage.setComponentAccesses({{}});
  // When:
  storage.clear();
  // Then:
  EXPECT_EQ(0, storage.getSourceLocationCount());
  EXPECT_THAT(storage.getStorageFiles(), IsEmpty());
  EXPECT_THAT(storage.getStorageSourceLocations(), IsEmpty());
  EXPECT_THAT(storage.getStorageOccurrences(), IsEmpty());
  EXPECT_THAT(storage.getComponentAccesses(), IsEmpty());
}

TEST(IntermediateStorageFix, hasFatalErrorsEmpty) {
  // Given:
  IntermediateStorage storage;
  // When:
  const auto result = storage.hasFatalErrors();
  // Then:
  EXPECT_FALSE(result);
}

TEST(IntermediateStorageFix, hasFatalErrorsNoError) {
  // Given:
  IntermediateStorage storage;
  storage.setErrors({{0, L"0.cpp", L"", false, true}, {1, L"1.cpp", L"", false, true}});
  // When:
  const auto result = storage.hasFatalErrors();
  // Then:
  EXPECT_FALSE(result);
}

TEST(IntermediateStorageFix, hasFatalErrors) {
  // Given:
  IntermediateStorage storage;
  storage.setErrors({{0, L"0.cpp", L"", true, true}, {1, L"1.cpp", L"", false, true}});
  // When:
  const auto result = storage.hasFatalErrors();
  // Then:
  EXPECT_TRUE(result);
}

TEST(IntermediateStorageFix, setAllFilesIncompleteEmpty) {
  // Given:
  IntermediateStorage storage;
  storage.setStorageFiles(std::vector<StorageFile>{
      StorageFile{3, L"main.cpp", L"cpp", "now", true, true},
      StorageFile{5, L"application.cpp", L"cpp", "now", false, false},
  });
  // When:
  storage.setAllFilesIncomplete();
  // Then:
  const auto files = storage.getStorageFiles();
  ASSERT_EQ(2, files.size());
  EXPECT_FALSE(files[0].complete);
  EXPECT_FALSE(files[1].complete);
}

TEST(IntermediateStorageFix, addNodeForFirstTime) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageNodeData node;
  const auto result = storage.addNode(node);
  // Then:
  ASSERT_TRUE(result.second);
  EXPECT_EQ(1, result.first);
  const auto nodes = storage.getStorageNodes();
  ASSERT_EQ(1, nodes.size());
  EXPECT_TRUE(node == nodes.front());
}

TEST(IntermediateStorageFix, addTwoNodesForSameNodeUsingAddNodes) {
  // Given: A node added to the storage
  IntermediateStorage storage;
  StorageNode node{};
  std::vector<StorageNode> nodes{node};
  node.type++;
  nodes.push_back(node);
  // When: Add the same node
  const auto result = storage.addNodes(nodes);
  // Then:
  ASSERT_EQ(2, result.size());
  const std::vector<Id> expectedIds{1, 1};
  ASSERT_EQ(result, expectedIds);
  const auto resultNodes = storage.getStorageNodes();
  ASSERT_EQ(1, resultNodes.size());
  EXPECT_TRUE(nodes.back() == resultNodes.front());
}

TEST(IntermediateStorageFix, addTwoNodesAnotherNodeUsingAddNodes) {
  // Given: A node added to the storage
  IntermediateStorage storage;
  // When: Add the same node
  StorageNode node0;
  StorageNode node1{1, 0, L"New"};
  const auto result = storage.addNodes(std::vector{node0, node1});
  // Then:
  ASSERT_EQ(2, result.size());
  const std::vector<Id> expectedIds{1, 2};
  ASSERT_EQ(result, expectedIds);
  const auto nodes = storage.getStorageNodes();
  ASSERT_EQ(2, nodes.size());
  EXPECT_TRUE(node0 == nodes[0]);
  EXPECT_TRUE(node1 == nodes[1]);
}

TEST(IntermediateStorageFix, setNodeTypeMissingNode) {
  // Given:
  IntermediateStorage storage;
  // When:
  storage.setNodeType(Id{std::numeric_limits<Id>::max()}, 0);
  // Then:
}

TEST(IntermediateStorageFix, setNodeTypeGoodCase) {
  // Given:
  IntermediateStorage storage;
  StorageNodeData node;
  const auto result = storage.addNode(node);
  // When:
  storage.setNodeType(result.first, 1);
  // Then:
  const auto nodes = storage.getStorageNodes();
  ASSERT_EQ(1, nodes.size());
  EXPECT_TRUE(node.serializedName == nodes[0].serializedName);
  EXPECT_TRUE(1 == nodes[0].type);
}

TEST(IntermediateStorageFix, setEmptyStorageNodes) {
  // Given:
  IntermediateStorage storage;
  // When:
  storage.setStorageNodes({});
  // Then:
  const auto result = storage.getStorageNodes();
  ASSERT_THAT(result, IsEmpty());
}

TEST(IntermediateStorageFix, setStorageNodes) {
  // Given:
  IntermediateStorage storage;
  std::vector<StorageNode> nodes{
      {1, {}},
      {2, {}},
      {3, {}},
      {1, {}},
  };
  // When:
  storage.setStorageNodes(nodes);
  // Then:
  const auto result = storage.getStorageNodes();
  ASSERT_EQ(nodes.size(), result.size());    // TODO: Check the behave
}

TEST(IntermediateStorageFix, AddFileFirstTime) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageFile file{};
  storage.addFile(file);
  // Then:
  const auto storageFiles = storage.getStorageFiles();
  ASSERT_EQ(1, storageFiles.size());
  EXPECT_EQ(file.filePath, storageFiles.front().filePath);
}

TEST(IntermediateStorageFix, addTwoFilesForSameFile) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageFile file{1, L"1.cpp", L"", {}, {}, {}};
  storage.addFile(file);
  file.languageIdentifier = L"cpp";
  storage.addFile(file);
  // Then:
  const auto storageFiles = storage.getStorageFiles();
  ASSERT_EQ(1, storageFiles.size());
  const auto& resultFile = storageFiles.front();
  EXPECT_EQ(file.filePath, resultFile.filePath);
  EXPECT_FALSE(resultFile.indexed);
  EXPECT_FALSE(resultFile.complete);
  EXPECT_EQ(file.languageIdentifier, resultFile.languageIdentifier);
}

TEST(IntermediateStorageFix, addFiles) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageFile file0{1, L"1.cpp", {}, {}, {}, {}};
  StorageFile file1{2, L"2.cpp", {}, {}, {}, {}};
  StorageFile file2{1, L"1.cpp", {}, {}, true, true};
  storage.addFile(file0);
  storage.addFile(file1);
  storage.addFile(file2);
  // Then:
  const auto storageFiles = storage.getStorageFiles();
  ASSERT_EQ(2, storageFiles.size());
  EXPECT_EQ(file0.filePath, storageFiles[0].filePath);
  EXPECT_EQ(file1.filePath, storageFiles[1].filePath);
}

TEST(IntermediateStorageFix, setFilesWithErrorsIncomplete) {
  // Given: a sourceLocation is set with LOCATION_ERROR
  IntermediateStorage storage;
  std::vector<StorageSourceLocation> storageSourceLocation{
      StorageSourceLocation{0, 3, 0, 0, 1, 1, locationTypeToInt(LOCATION_ERROR)},
      StorageSourceLocation{1, 4, 0, 0, 1, 1, locationTypeToInt(LOCATION_ERROR)},
      StorageSourceLocation{2, 5, 0, 0, 1, 1, locationTypeToInt(LOCATION_SIGNATURE)},
  };
  storage.addSourceLocations(storageSourceLocation);
  std::vector<StorageFile> storageFiles{
      StorageFile{3, L"main.cpp", L"cpp", "now", true, true},
      StorageFile{5, L"application.cpp", L"cpp", "now", false, false},
  };
  storage.setStorageFiles(storageFiles);
  // When:
  storage.setFilesWithErrorsIncomplete();
  // Then:
  const auto resultFiles = storage.getStorageFiles();
  ASSERT_EQ(2, resultFiles.size());
  EXPECT_FALSE(resultFiles[0].complete);
  EXPECT_FALSE(resultFiles[1].complete);
}

TEST(IntermediateStorageFix, setFileLanguageMissingNode) {
  // Given:
  IntermediateStorage storage;
  // When:
  storage.setFileLanguage(Id{std::numeric_limits<Id>::max()}, L"cpp");
  // Then:
  const auto nodes = storage.getStorageFiles();
  ASSERT_TRUE(nodes.empty());
}

TEST(IntermediateStorageFix, setFileLanguageGoodCase) {
  // Given:
  IntermediateStorage storage;
  StorageFile file{1, L"1.cpp", {}, {}, {}, {}};
  storage.addFile(file);
  // When:
  storage.setFileLanguage(Id{std::numeric_limits<Id>::max()}, L"cpp");
  // Then:
  const auto nodes = storage.getStorageFiles();
  ASSERT_EQ(1, nodes.size());
}

TEST(IntermediateStorageFix, setStorageFiles) {
  // Given:
  IntermediateStorage storage;
  std::vector<StorageFile> files{
      {},
      {},
      {10, L"", L"", "", false, false},
  };
  // When:
  storage.setStorageFiles(files);
  // Then:
  const auto result = storage.getStorageFiles();
  ASSERT_EQ(files.size(), result.size());    // TODO: Check the behave
}

TEST(IntermediateStorageFix, AddEdgeFirstTime) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageEdgeData edge{};
  const auto edgeId = storage.addEdge(edge);
  // Then:
  ASSERT_EQ(1, edgeId);
  const auto storageEdges = storage.getStorageEdges();
  ASSERT_EQ(1, storageEdges.size());
  EXPECT_TRUE(storageEdges.front().sourceNodeId == edge.sourceNodeId);
  EXPECT_TRUE(storageEdges.front().targetNodeId == edge.targetNodeId);
  EXPECT_TRUE(storageEdges.front().type == edge.type);
}

TEST(IntermediateStorageFix, addTwoEdgesForSameEdge) {
  // Given:
  IntermediateStorage storage;
  // When:
  std::vector<StorageEdge> edges = {{}, {}};
  const auto edgesId = storage.addEdges(edges);
  // Then:
  ASSERT_EQ(edgesId.size(), 2);
  EXPECT_EQ(1, edgesId[0]);
  EXPECT_EQ(1, edgesId[1]);
  const auto storageEdges = storage.getStorageEdges();
  ASSERT_EQ(1, storageEdges.size());
}

TEST(IntermediateStorageFix, addTwoEdges) {
  // Given:
  IntermediateStorage storage;
  // When:
  std::vector<StorageEdge> edges = {{1, 1, 100, 101}, {2, 2, 200, 201}};
  const auto edgesId = storage.addEdges(edges);
  // Then:
  ASSERT_EQ(edgesId.size(), 2);
  EXPECT_EQ(1, edgesId[0]);
  EXPECT_EQ(2, edgesId[1]);
  const auto storageEdges = storage.getStorageEdges();
  ASSERT_EQ(2, storageEdges.size());
}

TEST(IntermediateStorageFix, setStorageEdges) {
  // Given:
  IntermediateStorage storage;
  std::vector<StorageEdge> edges{
      {},
      {},
      {10, {}},
  };
  // When:
  storage.setStorageEdges(edges);
  // Then:
  const auto result = storage.getStorageEdges();
  ASSERT_EQ(edges.size(), result.size());    // TODO: Check the behave
}

TEST(IntermediateStorageFix, addLocalSymbolFirstTime) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageLocalSymbolData symbolData{};
  const auto symbolId = storage.addLocalSymbol(symbolData);
  // Then:
  ASSERT_EQ(1, symbolId);
  const auto storageLocalSymbols = storage.getStorageLocalSymbols();
  ASSERT_EQ(1, storageLocalSymbols.size());
}

TEST(IntermediateStorageFix, addLocalSymbol) {
  // Given:
  IntermediateStorage storage;
  // When:
  std::set<StorageLocalSymbol> localSymbols{{}, {2, L"something"}};
  const auto symbolId = storage.addLocalSymbols(localSymbols);
  // Then:
  ASSERT_EQ(2, symbolId.size());
  EXPECT_EQ(1, symbolId[0]);
  // EXPECT_EQ(0, symbolId[2]); random number
  const auto storageLocalSymbols = storage.getStorageLocalSymbols();
  ASSERT_EQ(2, storageLocalSymbols.size());
}

TEST(IntermediateStorageFix, addSourceLocations) {
  // Given:
  IntermediateStorage storage;
  // When:
  std::vector<StorageSourceLocation> sourceLocations{{}, {}, {1, 2, 0, 0, 1, 1, 1}};
  const auto sourceLocationIds = storage.addSourceLocations(sourceLocations);
  // Then:
  ASSERT_EQ(3, sourceLocationIds.size());
  EXPECT_EQ(1, sourceLocationIds[0]);
  EXPECT_EQ(1, sourceLocationIds[1]);
  EXPECT_EQ(2, sourceLocationIds[2]);
  const auto storageSourceLocations = storage.getStorageSourceLocations();
  ASSERT_EQ(2, storageSourceLocations.size());
}

TEST(IntermediateStorageFix, addErrors) {
  // Given:
  IntermediateStorage storage;
  StorageErrorData error0;
  StorageErrorData error1{L"message", L"translationUnit", {}, {}};
  // When:
  const auto error0Id = storage.addError(error0);
  const auto error1Id = storage.addError(error0);
  const auto error2Id = storage.addError(error1);
  // Then:
  EXPECT_EQ(1, error0Id);
  EXPECT_EQ(error0Id, error1Id);
  EXPECT_EQ(2, error2Id);
  const auto errors = storage.getErrors();
  ASSERT_EQ(2, errors.size());
}

TEST(IntermediateStorageFix, setEmptyErrors) {
  // Given:
  IntermediateStorage storage;
  // When:
  storage.setErrors({});
  // Then:
  const auto resultErrors = storage.getErrors();
  ASSERT_THAT(resultErrors, IsEmpty());
}

TEST(IntermediateStorageFix, setErrors) {
  // Given:
  IntermediateStorage storage;
  std::vector<StorageError> errors{{1, {}}, {2, {}}, {3, {}}};
  // When:
  storage.setErrors(errors);
  // Then:
  const auto resultErrors = storage.getErrors();
  ASSERT_EQ(errors.size(), resultErrors.size());
}

TEST(IntermediateStorageFix, addSymbolsEmpty) {
  // Given: one symbol exists
  IntermediateStorage storage;
  storage.addSymbol({});
  // When: add empty symbol
  storage.addSymbols({});
  // Then: the symbols didn't change
  EXPECT_EQ(1, storage.getStorageSymbols().size());
}

TEST(IntermediateStorageFix, addSymbols) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.addSymbols({{}, {}, {}});
  // Then: the symbols didn't change
  EXPECT_EQ(3, storage.getStorageSymbols().size());
}

TEST(IntermediateStorageFix, addOccurrencesEmpty) {
  // Given: one symbol exists
  IntermediateStorage storage;
  storage.addOccurrence({});
  // When: add empty symbol
  storage.addOccurrences({});
  // Then: the symbols didn't change
  EXPECT_EQ(1, storage.getStorageOccurrences().size());
}

TEST(IntermediateStorageFix, addOccurrences) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.addOccurrences({{}, {}, {1, 1}});
  // Then: the symbols didn't change
  EXPECT_EQ(2, storage.getStorageOccurrences().size());
}

TEST(IntermediateStorageFix, addComponentAccessesEmpty) {
  // Given: one symbol exists
  IntermediateStorage storage;
  storage.addComponentAccess({});
  // When: add empty symbol
  storage.addComponentAccesses({});
  // Then: the symbols didn't change
  EXPECT_EQ(1, storage.getComponentAccesses().size());
}

TEST(IntermediateStorageFix, addComponentAccess) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.addComponentAccesses({{0, 0}, {0, 1}, {1, 1}});
  // Then: the symbols didn't change
  EXPECT_EQ(2, storage.getComponentAccesses().size());
}

TEST(IntermediateStorageFix, addElementComponentsEmpty) {
  // Given: one symbol exists
  IntermediateStorage storage;
  storage.addElementComponent({});
  // When: add empty symbol
  storage.addElementComponents({});
  // Then: the symbols didn't change
  EXPECT_EQ(1, storage.getElementComponents().size());
}

TEST(IntermediateStorageFix, addElementComponents) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.addElementComponents({{}, {}, {1, 1, {}}});
  // Then: the symbols didn't change
  EXPECT_EQ(2, storage.getElementComponents().size());
}

TEST(IntermediateStorageFix, setStorageSymbolsEmpty) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.setStorageSymbols({});
  // Then: the symbols didn't change
  EXPECT_THAT(storage.getStorageSymbols(), IsEmpty());
}

TEST(IntermediateStorageFix, setStorageLocalSymbols) {
  // Given: no symbols exists
  IntermediateStorage storage;
  storage.addLocalSymbol({L"something"});
  // When: add a list of symbols
  storage.setStorageLocalSymbols({});
  // Then: the symbols didn't change
  EXPECT_THAT(storage.getStorageLocalSymbols(), IsEmpty());
}

TEST(IntermediateStorageFix, NextId) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  const auto nextId = storage.getNextId();
  // Then: the symbols didn't change
  EXPECT_EQ(nextId, 1);
}

TEST(IntermediateStorageFix, SetNextId) {
  // Given: no symbols exists
  IntermediateStorage storage;
  // When: add a list of symbols
  storage.setNextId(10);
  // Then: the symbols didn't change
  EXPECT_EQ(storage.getNextId(), 10);
}
}    // namespace