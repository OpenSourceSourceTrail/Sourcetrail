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
  return ParseLocation(1, 1, locationId, 1, locationId);
}

NameHierarchy createNameHierarchy(std::wstring s) {
  NameHierarchy nameHierarchy(NAME_DELIMITER_CXX);
  for(std::wstring element : utility::splitToVector(s, nameDelimiterTypeToString(NAME_DELIMITER_CXX))) {
    nameHierarchy.push(element);
  }
  return nameHierarchy;
}

[[maybe_unused]] NameHierarchy createFunctionNameHierarchy(std::wstring ret, std::wstring name, std::wstring parameters) {
  NameHierarchy nameHierarchy = createNameHierarchy(name);
  std::wstring lastName = nameHierarchy.back().getName();
  nameHierarchy.pop();
  nameHierarchy.push(NameElement(lastName, ret, parameters));
  return nameHierarchy;
}

TEST(Storage, savesFile) {
  TestStorage storage;

  std::wstring filePath = L"path/to/test.h";

  std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();
  Id id = intermediateStorage
              ->addNode(StorageNodeData(
                  nodeKindToInt(NODE_FILE), NameHierarchy::serialize(NameHierarchy(filePath, NAME_DELIMITER_FILE))))
              .first;
  intermediateStorage->addFile(StorageFile(id, filePath, L"someLanguage", "someTime", true, true));

  storage.inject(intermediateStorage.get());

  EXPECT_TRUE(storage.getNameHierarchyForNodeId(id).getQualifiedName() == filePath);
  EXPECT_TRUE(storage.getNodeTypeForNodeWithId(id).isFile());
}

TEST(Storage, savesNode) {
  NameHierarchy a = createNameHierarchy(L"type");

  TestStorage storage;

  std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();
  intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_TYPEDEF), NameHierarchy::serialize(a)));

  storage.inject(intermediateStorage.get());

  Id storedId = storage.getNodeIdForNameHierarchy(a);

  EXPECT_TRUE(storedId != 0);
  EXPECT_TRUE(storage.getNodeTypeForNodeWithId(storedId).getKind() == NODE_TYPEDEF);
}

TEST(Storage, savesFieldAsMember) {
  NameHierarchy a = createNameHierarchy(L"Struct");
  NameHierarchy b = createNameHierarchy(L"Struct::m_field");

  TestStorage storage;

  std::shared_ptr<IntermediateStorage> intermediateStorage = std::make_shared<IntermediateStorage>();

  Id aId = intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_STRUCT), NameHierarchy::serialize(a))).first;
  intermediateStorage->addSymbol(StorageSymbol(aId, DEFINITION_EXPLICIT));

  Id bId = intermediateStorage->addNode(StorageNodeData(nodeKindToInt(NODE_FIELD), NameHierarchy::serialize(b))).first;
  intermediateStorage->addSymbol(StorageSymbol(bId, DEFINITION_EXPLICIT));
  intermediateStorage->addEdge(StorageEdgeData(Edge::typeToInt(Edge::EDGE_MEMBER), aId, bId));

  storage.inject(intermediateStorage.get());
  bool foundEdge = false;

  const Id sourceId = storage.getNodeIdForNameHierarchy(a);
  const Id targetId = storage.getNodeIdForNameHierarchy(b);
  for(auto edge : storage.getStorageEdges()) {
    if(edge.sourceNodeId == sourceId && edge.targetNodeId == targetId && edge.type == Edge::typeToInt(Edge::EDGE_MEMBER)) {
      foundEdge = true;
    }
  }
  EXPECT_TRUE(foundEdge);
}
}    // namespace
