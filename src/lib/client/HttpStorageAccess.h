#pragma once
#include "StorageAccess.h"

class EngineChannel;

/**
 * StorageAccess backed by the engine daemon rather than by SQLite.
 *
 * This is the whole of the client's read path: install it as the StorageAccessProxy's subject and
 * every controller and view queries the engine over HTTP without knowing it.
 *
 * A failed request never propagates. Each override returns an empty result -- no graph, no locations, no
 * matches -- and marks the channel degraded so the supervisor can restart the engine. Views then
 * render an empty state instead of the process dying, which is what keeps the GUI alive across an
 * engine crash. Callers cannot distinguish "engine is down" from "nothing found" and must not need
 * to; ask EngineChannel::isConnected() if that distinction matters.
 */
class HttpStorageAccess final : public StorageAccess {
public:
  explicit HttpStorageAccess(EngineChannel* channel);

  Id getNodeIdForFileNode(const FilePath& filePath) const override;
  Id getNodeIdForNameHierarchy(const NameHierarchy& nameHierarchy) const override;
  std::vector<Id> getNodeIdsForNameHierarchies(const std::vector<NameHierarchy> nameHierarchies) const override;

  NameHierarchy getNameHierarchyForNodeId(Id nodeId) const override;
  std::vector<NameHierarchy> getNameHierarchiesForNodeIds(const std::vector<Id>& nodeIds) const override;
  std::map<Id, std::pair<Id, NameHierarchy>> getNodeIdToParentFileMap(const std::vector<Id>& nodeIds) const override;

  NodeType getNodeTypeForNodeWithId(Id nodeId) const override;

  StorageEdge getEdgeById(Id edgeId) const override;

  std::shared_ptr<SourceLocationCollection> getFullTextSearchLocations(const std::wstring& searchTerm,
                                                                       bool caseSensitive) const override;
  std::vector<SearchMatch> getAutocompletionMatches(const std::wstring& query,
                                                    NodeTypeSet acceptedNodeTypes,
                                                    bool acceptCommands) const override;
  std::vector<SearchMatch> getSearchMatchesForTokenIds(const std::vector<Id>& tokenIds) const override;

  std::shared_ptr<Graph> getGraphForAll() const override;
  std::shared_ptr<Graph> getGraphForNodeTypes(NodeTypeSet nodeTypes) const override;
  std::shared_ptr<Graph> getGraphForActiveTokenIds(const std::vector<Id>& tokenIds,
                                                   const std::vector<Id>& expandedNodeIds,
                                                   bool* isActiveNamespace = nullptr) const override;
  std::shared_ptr<Graph> getGraphForChildrenOfNodeId(Id nodeId) const override;
  std::shared_ptr<Graph> getGraphForTrail(Id originId,
                                          Id targetId,
                                          NodeKindMask nodeTypes,
                                          Edge::TypeMask edgeTypes,
                                          bool nodeNonIndexed,
                                          size_t depth,
                                          bool directed) const override;

  NodeKindMask getAvailableNodeTypes() const override;
  Edge::TypeMask getAvailableEdgeTypes() const override;

  std::vector<Id> getActiveTokenIdsForId(Id tokenId, Id* declarationId) const override;
  std::vector<Id> getNodeIdsForLocationIds(const std::vector<Id>& locationIds) const override;

  std::shared_ptr<SourceLocationCollection> getSourceLocationsForTokenIds(const std::vector<Id>& tokenIds) const override;
  std::shared_ptr<SourceLocationCollection> getSourceLocationsForLocationIds(const std::vector<Id>& locationIds) const override;

  std::shared_ptr<SourceLocationFile> getSourceLocationsForFile(const FilePath& filePath) const override;
  std::shared_ptr<SourceLocationFile> getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                       size_t startLine,
                                                                       size_t endLine) const override;
  std::shared_ptr<SourceLocationFile> getSourceLocationsOfTypeInFile(const FilePath& filePath, LocationType type) const override;

  std::shared_ptr<TextAccess> getFileContent(const FilePath& filePath, bool showsErrors) const override;

  FileInfo getFileInfoForFileId(Id fileId) const override;
  FileInfo getFileInfoForFilePath(const FilePath& filePath) const override;
  std::vector<FileInfo> getFileInfosForFilePaths(const std::vector<FilePath>& filePaths) const override;

  StorageStats getStorageStats() const override;

  ErrorCountInfo getErrorCount() const override;
  std::vector<ErrorInfo> getErrorsLimited(const ErrorFilter& filter) const override;
  std::vector<ErrorInfo> getErrorsForFileLimited(const ErrorFilter& filter, const FilePath& filePath) const override;
  std::shared_ptr<SourceLocationCollection> getErrorSourceLocations(const std::vector<ErrorInfo>& errors) const override;

  Id addNodeBookmark(const NodeBookmark& bookmark) override;
  Id addEdgeBookmark(const EdgeBookmark& bookmark) override;
  Id addBookmarkCategory(const std::wstring& categoryName) override;
  void updateBookmark(const Id bookmarkId,
                      const std::wstring& name,
                      const std::wstring& comment,
                      const std::wstring& categoryName) override;
  void removeBookmark(const Id bookmarkId) override;
  void removeBookmarkCategory(const Id bookmarkCategoryId) override;
  std::vector<NodeBookmark> getAllNodeBookmarks() const override;
  std::vector<EdgeBookmark> getAllEdgeBookmarks() const override;
  std::vector<BookmarkCategory> getAllBookmarkCategories() const override;

  TooltipInfo getTooltipInfoForTokenIds(const std::vector<Id>& tokenIds, TooltipOrigin origin) const override;
  TooltipInfo getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(const std::vector<Id>& locationIds,
                                                                  const std::vector<Id>& localSymbolIds) const override;

private:
  // Every override goes through the file-local `call()` in the .cpp, which is the single place an
  // engine failure is turned into an empty answer. Keeping it out of this header keeps gRPC out of
  // it too, so GUI translation units including this never see grpc headers.
  EngineChannel* mChannel;
};
