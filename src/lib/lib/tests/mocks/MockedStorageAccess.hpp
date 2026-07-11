#pragma once
#include <gmock/gmock.h>

#include "FileInfo.h"
#include "NodeTypeSet.h"
#include "StorageAccess.h"

class MockedStorageAccess : public StorageAccess {
public:
  using Ids = std::vector<Id>;
  using NameHierarchies = std::vector<NameHierarchy>;
  using MapIdToName = std::map<Id, std::pair<Id, NameHierarchy>>;
  using SourceLocationCollectionPtr = std::shared_ptr<SourceLocationCollection>;
  using SearchMatchs = std::vector<SearchMatch>;
  using GraphPtr = std::shared_ptr<Graph>;
  using TypeMask = Edge::TypeMask;
  using SourceLocationFilePtr = std::shared_ptr<SourceLocationFile>;
  using TextAccessPtr = std::shared_ptr<TextAccess>;
  using VecFileInfo = std::vector<FileInfo>;
  using VecErrorInfo = std::vector<ErrorInfo>;
  using NodeBookmarks = std::vector<NodeBookmark>;
  using EdgeBookmarks = std::vector<EdgeBookmark>;
  using BookmarkCategories = std::vector<BookmarkCategory>;
  using FilePaths = std::vector<FilePath>;

  ~MockedStorageAccess() override = default;

  MOCK_METHOD(Id, getNodeIdForFileNode, (const FilePath&), (const, override));

  MOCK_METHOD(Id, getNodeIdForNameHierarchy, (const NameHierarchy&), (const, override));

  MOCK_METHOD(Ids, getNodeIdsForNameHierarchies, (const NameHierarchies), (const, override));

  MOCK_METHOD(NameHierarchy, getNameHierarchyForNodeId, (Id), (const, override));

  MOCK_METHOD(NameHierarchies, getNameHierarchiesForNodeIds, (const Ids&), (const, override));

  MOCK_METHOD(MapIdToName, getNodeIdToParentFileMap, (const Ids&), (const, override));

  MOCK_METHOD(NodeType, getNodeTypeForNodeWithId, (Id), (const, override));

  MOCK_METHOD(StorageEdge, getEdgeById, (Id), (const, override));

  MOCK_METHOD(SourceLocationCollectionPtr, getFullTextSearchLocations, (const std::wstring&, bool), (const, override));

  MOCK_METHOD(SearchMatchs, getAutocompletionMatches, (const std::wstring&, NodeTypeSet, bool), (const, override));

  MOCK_METHOD(SearchMatchs, getSearchMatchesForTokenIds, (const Ids&), (const, override));

  MOCK_METHOD(GraphPtr, getGraphForAll, (), (const, override));

  MOCK_METHOD(GraphPtr, getGraphForNodeTypes, (NodeTypeSet), (const, override));

  MOCK_METHOD(GraphPtr, getGraphForActiveTokenIds, (const Ids&, const Ids&, bool*), (const, override));

  MOCK_METHOD(GraphPtr, getGraphForChildrenOfNodeId, (Id), (const, override));

  MOCK_METHOD(GraphPtr, getGraphForTrail, (Id, Id, NodeKindMask, TypeMask, bool, size_t, bool), (const, override));

  MOCK_METHOD(NodeKindMask, getAvailableNodeTypes, (), (const, override));

  MOCK_METHOD(TypeMask, getAvailableEdgeTypes, (), (const, override));

  MOCK_METHOD(Ids, getActiveTokenIdsForId, (Id, Id*), (const, override));

  MOCK_METHOD(Ids, getNodeIdsForLocationIds, (const Ids&), (const, override));

  MOCK_METHOD(SourceLocationCollectionPtr, getSourceLocationsForTokenIds, (const Ids&), (const, override));

  MOCK_METHOD(SourceLocationCollectionPtr, getSourceLocationsForLocationIds, (const Ids&), (const, override));

  MOCK_METHOD(SourceLocationFilePtr, getSourceLocationsForFile, (const FilePath&), (const, override));

  MOCK_METHOD(SourceLocationFilePtr, getSourceLocationsForLinesInFile, (const FilePath&, size_t, size_t), (const, override));

  MOCK_METHOD(SourceLocationFilePtr, getSourceLocationsOfTypeInFile, (const FilePath&, LocationType), (const, override));

  MOCK_METHOD(TextAccessPtr, getFileContent, (const FilePath&, bool), (const, override));

  MOCK_METHOD(FileInfo, getFileInfoForFileId, (Id), (const, override));

  MOCK_METHOD(FileInfo, getFileInfoForFilePath, (const FilePath&), (const, override));

  MOCK_METHOD(VecFileInfo, getFileInfosForFilePaths, (const FilePaths&), (const, override));

  MOCK_METHOD(StorageStats, getStorageStats, (), (const, override));

  MOCK_METHOD(ErrorCountInfo, getErrorCount, (), (const, override));

  MOCK_METHOD(VecErrorInfo, getErrorsLimited, (const ErrorFilter&), (const, override));

  MOCK_METHOD(VecErrorInfo, getErrorsForFileLimited, (const ErrorFilter&, const FilePath&), (const, override));

  MOCK_METHOD(SourceLocationCollectionPtr, getErrorSourceLocations, (const VecErrorInfo&), (const, override));

  MOCK_METHOD(Id, addNodeBookmark, (const NodeBookmark&), (override));

  MOCK_METHOD(Id, addEdgeBookmark, (const EdgeBookmark&), (override));

  MOCK_METHOD(Id, addBookmarkCategory, (const std::wstring&), (override));

  MOCK_METHOD(void, updateBookmark, (const Id, const std::wstring&, const std::wstring&, const std::wstring&), (override));

  MOCK_METHOD(void, removeBookmark, (const Id), (override));

  MOCK_METHOD(void, removeBookmarkCategory, (const Id), (override));

  MOCK_METHOD(NodeBookmarks, getAllNodeBookmarks, (), (const, override));

  MOCK_METHOD(EdgeBookmarks, getAllEdgeBookmarks, (), (const, override));

  MOCK_METHOD(BookmarkCategories, getAllBookmarkCategories, (), (const, override));

  MOCK_METHOD(TooltipInfo, getTooltipInfoForTokenIds, (const Ids&, TooltipOrigin), (const, override));

  MOCK_METHOD(TooltipInfo, getTooltipInfoForSourceLocationIdsAndLocalSymbolIds, (const Ids&, const Ids&), (const, override));

  MOCK_METHOD(void, setUseErrorCache, (bool), (override));

  MOCK_METHOD(void, addErrorsToCache, (const VecErrorInfo&, const ErrorCountInfo&), (override));
};
