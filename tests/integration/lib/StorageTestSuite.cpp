#include <memory>
#include <utility>

#include <gtest/gtest.h>

#include "IntermediateStorage.h"
#include "ParseLocation.h"
#include "PersistentStorage.h"
#include "utilityString.h"

namespace {
class TestStorage : public PersistentStorage {
public:
  TestStorage() : PersistentStorage(FilePath(L"data/test.sqlite"), FilePath(L"data/testBookmarks.sqlite")) {
    clear();
  }
};

[[maybe_unused]] ParseLocation validLocation(Id locationId = 0) {
  return {1, 1, locationId, 1, locationId};
}

NameHierarchy createNameHierarchy(const std::wstring& name) {
  NameHierarchy nameHierarchy(NAME_DELIMITER_CXX);
  for(const std::wstring& element : utility::splitToVector(name, nameDelimiterTypeToString(NAME_DELIMITER_CXX))) {
    nameHierarchy.push(element);
  }
  return nameHierarchy;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
[[maybe_unused]] NameHierarchy createFunctionNameHierarchy(std::wstring ret, const std::wstring& name, std::wstring parameters) {
  NameHierarchy nameHierarchy = createNameHierarchy(name);
  std::wstring lastName = nameHierarchy.back().getName();
  nameHierarchy.pop();
  nameHierarchy.push(NameElement(std::move(lastName), std::move(ret), std::move(parameters)));
  return nameHierarchy;
}

TEST(Storage, savesFile) {
  TestStorage storage;

  const std::wstring filePath = L"path/to/test.h";

  const std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();
  const Id nodeId = intermediateStorage
                        ->addNode(StorageNodeData(
                            nodeKindToInt(NODE_FILE), NameHierarchy::serialize(NameHierarchy(filePath, NAME_DELIMITER_FILE))))
                        .first;
  intermediateStorage->addFile(StorageFile(nodeId, filePath, L"someLanguage", "someTime", true, true));

  storage.inject(intermediateStorage.get());

  EXPECT_TRUE(storage.getNameHierarchyForNodeId(nodeId).getQualifiedName() == filePath);
  EXPECT_TRUE(storage.getNodeTypeForNodeWithId(nodeId).isFile());
}

TEST(Storage, savesNode) {
  const NameHierarchy nameHierarchy = createNameHierarchy(L"type");

  TestStorage storage;

  const std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();
  intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_TYPEDEF), NameHierarchy::serialize(nameHierarchy)));

  storage.inject(intermediateStorage.get());

  const Id storedId = storage.getNodeIdForNameHierarchy(nameHierarchy);

  EXPECT_TRUE(storedId != 0);
  EXPECT_TRUE(storage.getNodeTypeForNodeWithId(storedId).getKind() == NODE_TYPEDEF);
}

TEST(Storage, savesFieldAsMember) {
  const NameHierarchy nameHierarchyA = createNameHierarchy(L"Struct");
  const NameHierarchy nameHierarchyB = createNameHierarchy(L"Struct::m_field");

  TestStorage storage;

  const std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();

  const Id aId =
      intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_STRUCT), NameHierarchy::serialize(nameHierarchyA))).first;
  intermediateStorage->addSymbol(StorageSymbol(aId, DEFINITION_EXPLICIT));

  const Id bId =
      intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_FIELD), NameHierarchy::serialize(nameHierarchyB))).first;
  intermediateStorage->addSymbol(StorageSymbol(bId, DEFINITION_EXPLICIT));
  intermediateStorage->addEdge(StorageEdgeData(Edge::typeToInt(Edge::EDGE_MEMBER), aId, bId));

  storage.inject(intermediateStorage.get());
  bool foundEdge = false;

  const Id sourceId = storage.getNodeIdForNameHierarchy(nameHierarchyA);
  const Id targetId = storage.getNodeIdForNameHierarchy(nameHierarchyB);
  for(auto edge : storage.getStorageEdges()) {
    if(edge.sourceNodeId == sourceId && edge.targetNodeId == targetId && edge.type == Edge::typeToInt(Edge::EDGE_MEMBER)) {
      foundEdge = true;
    }
  }
  EXPECT_TRUE(foundEdge);
}
}    // namespace
