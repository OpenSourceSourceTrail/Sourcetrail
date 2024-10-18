#pragma once
/**
 * @file StorageAccess.h
 * @brief Defines the StorageAccess interface for accessing and manipulating stored data.
 */
#include <memory>
#include <string>
#include <vector>

#include "BookmarkCategory.h"
#include "EdgeBookmark.h"
#include "ErrorCountInfo.h"
#include "ErrorFilter.h"
#include "ErrorInfo.h"
#include "GlobalId.hpp"
#include "LocationType.h"
#include "NodeBookmark.h"
#include "SearchMatch.h"
#include "StorageEdge.h"
#include "StorageStats.h"
#include "TooltipInfo.h"
#include "TooltipOrigin.h"

class FilePath;
class Graph;
class NodeTypeSet;
class SourceLocationCollection;
class SourceLocationFile;
class TextAccess;
struct FileInfo;

/**
 * @class StorageAccess
 * @brief Abstract interface for accessing and manipulating stored data.
 *
 * This class provides methods for retrieving and manipulating various types of data,
 * including nodes, edges, source locations, and bookmarks.
 */
class StorageAccess {
public:
  virtual ~StorageAccess() = default;

  /**
   * @brief Get the node ID for a file node.
   * @param filePath The path of the file.
   * @return The ID of the node representing the file.
   */
  virtual Id getNodeIdForFileNode(const FilePath& filePath) const = 0;

  /**
   * @brief Get the node ID for a name hierarchy.
   * @param nameHierarchy The name hierarchy to look up.
   * @return The ID of the node representing the name hierarchy.
   */
  virtual Id getNodeIdForNameHierarchy(const NameHierarchy& nameHierarchy) const = 0;

  /**
   * @brief Get node IDs for multiple name hierarchies.
   * @param nameHierarchies A vector of name hierarchies to look up.
   * @return A vector of node IDs corresponding to the input name hierarchies.
   */
  virtual std::vector<Id> getNodeIdsForNameHierarchies(const std::vector<NameHierarchy> nameHierarchies) const = 0;

  /**
   * @brief Get the name hierarchy for a node ID.
   * @param id The ID of the node.
   * @return The name hierarchy of the node.
   */
  virtual NameHierarchy getNameHierarchyForNodeId(Id id) const = 0;

  /**
   * @brief Get name hierarchies for multiple node IDs.
   * @param nodeIds A vector of node IDs to look up.
   * @return A vector of name hierarchies corresponding to the input node IDs.
   */
  virtual std::vector<NameHierarchy> getNameHierarchiesForNodeIds(const std::vector<Id>& nodeIds) const = 0;

  /**
   * @brief Get a map of node IDs to their parent file information.
   * @param nodeIds A vector of node IDs to look up.
   * @return A map of node IDs to pairs of parent file ID and name hierarchy.
   */
  virtual std::map<Id, std::pair<Id, NameHierarchy>> getNodeIdToParentFileMap(const std::vector<Id>& nodeIds) const = 0;

  /**
   * @brief Get the node type for a given node ID.
   * @param id The ID of the node.
   * @return The type of the node.
   */
  virtual NodeType getNodeTypeForNodeWithId(Id id) const = 0;

  /**
   * @brief Get an edge by its ID.
   * @param edgeId The ID of the edge.
   * @return The StorageEdge object representing the edge.
   */
  virtual StorageEdge getEdgeById(Id edgeId) const = 0;

  /**
   * @brief Perform a full-text search and return matching locations.
   * @param searchTerm The term to search for.
   * @param caseSensitive Whether the search should be case-sensitive.
   * @return A shared pointer to a SourceLocationCollection containing the matching locations.
   */
  virtual std::shared_ptr<SourceLocationCollection> getFullTextSearchLocations(const std::wstring& searchTerm,
                                                                               bool caseSensitive) const = 0;

  /**
   * @brief Get autocompletion matches for a query.
   * @param query The query string.
   * @param acceptedNodeTypes Set of acceptable node types.
   * @param acceptCommands Whether to accept commands in the results.
   * @return A vector of SearchMatch objects representing the matches.
   */
  virtual std::vector<SearchMatch> getAutocompletionMatches(const std::wstring& query,
                                                            NodeTypeSet acceptedNodeTypes,
                                                            bool acceptCommands) const = 0;

  /**
   * @brief Get search matches for given token IDs.
   * @param tokenIds A vector of token IDs to look up.
   * @return A vector of SearchMatch objects representing the matches.
   */
  virtual std::vector<SearchMatch> getSearchMatchesForTokenIds(const std::vector<Id>& tokenIds) const = 0;

  /**
   * @brief Get a graph containing all nodes and edges.
   * @return A shared pointer to a Graph object representing the entire graph.
   */
  virtual std::shared_ptr<Graph> getGraphForAll() const = 0;

  /**
   * @brief Get a graph containing nodes of specified types.
   * @param nodeTypes Set of node types to include in the graph.
   * @return A shared pointer to a Graph object representing the filtered graph.
   */
  virtual std::shared_ptr<Graph> getGraphForNodeTypes(NodeTypeSet nodeTypes) const = 0;

  /**
   * @brief Get a graph for active token IDs and expanded node IDs.
   * @param tokenIds Vector of active token IDs.
   * @param expandedNodeIds Vector of expanded node IDs.
   * @param isActiveNamespace Pointer to a bool indicating if it's an active namespace.
   * @return A shared pointer to a Graph object representing the active graph.
   */
  virtual std::shared_ptr<Graph> getGraphForActiveTokenIds(const std::vector<Id>& tokenIds,
                                                           const std::vector<Id>& expandedNodeIds,
                                                           bool* isActiveNamespace = nullptr) const = 0;

  /**
   * @brief Get a graph containing children of a specified node.
   * @param nodeId The ID of the parent node.
   * @return A shared pointer to a Graph object representing the children graph.
   */
  virtual std::shared_ptr<Graph> getGraphForChildrenOfNodeId(Id nodeId) const = 0;

  /**
   * @brief Get a graph representing a trail between two nodes.
   * @param originId The ID of the origin node.
   * @param targetId The ID of the target node.
   * @param nodeTypes Mask specifying allowed node types.
   * @param edgeTypes Mask specifying allowed edge types.
   * @param nodeNonIndexed Whether to include non-indexed nodes.
   * @param depth Maximum depth of the trail.
   * @param directed Whether the trail should be directed.
   * @return A shared pointer to a Graph object representing the trail.
   */
  virtual std::shared_ptr<Graph> getGraphForTrail(Id originId,
                                                  Id targetId,
                                                  NodeKindMask nodeTypes,
                                                  Edge::TypeMask edgeTypes,
                                                  bool nodeNonIndexed,
                                                  size_t depth,
                                                  bool directed) const = 0;

  /**
   * @brief Get the available node types in the storage.
   * @return A NodeKindMask representing the available node types.
   */
  virtual NodeKindMask getAvailableNodeTypes() const = 0;

  /**
   * @brief Get the available edge types in the storage.
   * @return An Edge::TypeMask representing the available edge types.
   */
  virtual Edge::TypeMask getAvailableEdgeTypes() const = 0;

  /**
   * @brief Get active token IDs for a given token ID.
   * @param tokenId The ID of the token.
   * @param declarationId Pointer to store the declaration ID.
   * @return A vector of active token IDs.
   */
  virtual std::vector<Id> getActiveTokenIdsForId(Id tokenId, Id* declarationId) const = 0;

  /**
   * @brief Get node IDs for given location IDs.
   * @param locationIds A vector of location IDs.
   * @return A vector of corresponding node IDs.
   */
  virtual std::vector<Id> getNodeIdsForLocationIds(const std::vector<Id>& locationIds) const = 0;

  /**
   * @brief Get source locations for given token IDs.
   * @param tokenIds A vector of token IDs.
   * @return A shared pointer to a SourceLocationCollection containing the locations.
   */
  virtual std::shared_ptr<SourceLocationCollection> getSourceLocationsForTokenIds(const std::vector<Id>& tokenIds) const = 0;

  /**
   * @brief Get source locations for given location IDs.
   * @param locationIds A vector of location IDs.
   * @return A shared pointer to a SourceLocationCollection containing the locations.
   */
  virtual std::shared_ptr<SourceLocationCollection> getSourceLocationsForLocationIds(const std::vector<Id>& locationIds) const = 0;

  /**
   * @brief Get all source locations for a file.
   * @param filePath The path of the file.
   * @return A shared pointer to a SourceLocationFile containing the locations.
   */
  virtual std::shared_ptr<SourceLocationFile> getSourceLocationsForFile(const FilePath& filePath) const = 0;

  /**
   * @brief Get source locations for specific lines in a file.
   * @param filePath The path of the file.
   * @param startLine The starting line number.
   * @param endLine The ending line number.
   * @return A shared pointer to a SourceLocationFile containing the locations.
   */
  virtual std::shared_ptr<SourceLocationFile> getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                               size_t startLine,
                                                                               size_t endLine) const = 0;

  /**
   * @brief Get source locations of a specific type in a file.
   * @param filePath The path of the file.
   * @param type The type of locations to retrieve.
   * @return A shared pointer to a SourceLocationFile containing the locations.
   */
  virtual std::shared_ptr<SourceLocationFile> getSourceLocationsOfTypeInFile(const FilePath& filePath, LocationType type) const = 0;

  /**
   * @brief Get the content of a file.
   * @param filePath The path of the file.
   * @param showsErrors Whether to include error information.
   * @return A shared pointer to a TextAccess object containing the file content.
   */
  virtual std::shared_ptr<TextAccess> getFileContent(const FilePath& filePath, bool showsErrors) const = 0;

  /**
   * @brief Get file information for a given file ID.
   * @param id The ID of the file.
   * @return A FileInfo object containing information about the file.
   */
  virtual FileInfo getFileInfoForFileId(Id id) const = 0;

  /**
   * @brief Get file information for a given file path.
   * @param filePath The path of the file.
   * @return A FileInfo object containing information about the file.
   */
  virtual FileInfo getFileInfoForFilePath(const FilePath& filePath) const = 0;

  /**
   * @brief Get file information for multiple file paths.
   * @param filePaths A vector of file paths.
   * @return A vector of FileInfo objects containing information about the files.
   */
  virtual std::vector<FileInfo> getFileInfosForFilePaths(const std::vector<FilePath>& filePaths) const = 0;

  /**
   * @brief Get storage statistics.
   * @return A StorageStats object containing storage statistics.
   */
  virtual StorageStats getStorageStats() const = 0;

  /**
   * @brief Get the error count information.
   * @return An ErrorCountInfo object containing error count information.
   */
  virtual ErrorCountInfo getErrorCount() const = 0;

  /**
   * @brief Get a limited number of errors based on a filter.
   * @param filter An ErrorFilter object specifying the filter criteria.
   * @return A vector of ErrorInfo objects representing the filtered errors.
   */
  virtual std::vector<ErrorInfo> getErrorsLimited(const ErrorFilter& filter) const = 0;

  /**
   * @brief Get a limited number of errors for a specific file based on a filter.
   * @param filter An ErrorFilter object specifying the filter criteria.
   * @param filePath The path of the file.
   * @return A vector of ErrorInfo objects representing the filtered errors for the file.
   */
  virtual std::vector<ErrorInfo> getErrorsForFileLimited(const ErrorFilter& filter, const FilePath& filePath) const = 0;

  /**
   * @brief Get source locations for given errors.
   * @param errors A vector of ErrorInfo objects.
   * @return A shared pointer to a SourceLocationCollection containing the error locations.
   */
  virtual std::shared_ptr<SourceLocationCollection> getErrorSourceLocations(const std::vector<ErrorInfo>& errors) const = 0;

  /**
   * @brief Add a node bookmark.
   * @param bookmark The NodeBookmark to add.
   * @return The ID of the added bookmark.
   */
  virtual Id addNodeBookmark(const NodeBookmark& bookmark) = 0;

  /**
   * @brief Add an edge bookmark.
   * @param bookmark The EdgeBookmark to add.
   * @return The ID of the added bookmark.
   */
  virtual Id addEdgeBookmark(const EdgeBookmark& bookmark) = 0;

  /**
   * @brief Add a bookmark category.
   * @param categoryName The name of the category to add.
   * @return The ID of the added category.
   */
  virtual Id addBookmarkCategory(const std::wstring& categoryName) = 0;

  /**
   * @brief Update an existing bookmark.
   * @param bookmarkId The ID of the bookmark to update.
   * @param name The new name for the bookmark.
   * @param comment The new comment for the bookmark.
   * @param categoryName The new category name for the bookmark.
   */
  virtual void updateBookmark(const Id bookmarkId,
                              const std::wstring& name,
                              const std::wstring& comment,
                              const std::wstring& categoryName) = 0;

  /**
   * @brief Remove a bookmark.
   * @param id The ID of the bookmark to remove.
   */
  virtual void removeBookmark(const Id id) = 0;

  /**
   * @brief Remove a bookmark category.
   * @param id The ID of the category to remove.
   */
  virtual void removeBookmarkCategory(const Id id) = 0;

  /**
   * @brief Get all node bookmarks.
   * @return A vector of all NodeBookmark objects.
   */
  virtual std::vector<NodeBookmark> getAllNodeBookmarks() const = 0;

  /**
   * @brief Get all edge bookmarks.
   * @return A vector of all EdgeBookmark objects.
   */
  virtual std::vector<EdgeBookmark> getAllEdgeBookmarks() const = 0;

  /**
   * @brief Get all bookmark categories.
   * @return A vector of all BookmarkCategory objects.
   */
  virtual std::vector<BookmarkCategory> getAllBookmarkCategories() const = 0;

  /**
   * @brief Get tooltip information for given token IDs.
   * @param tokenIds A vector of token IDs.
   * @param origin The origin of the tooltip request.
   * @return A TooltipInfo object containing the tooltip information.
   */
  virtual TooltipInfo getTooltipInfoForTokenIds(const std::vector<Id>& tokenIds, TooltipOrigin origin) const = 0;

  /**
   * @brief Get tooltip information for given source location IDs and local symbol IDs.
   * @param locationIds A vector of source location IDs.
   * @param localSymbolIds A vector of local symbol IDs.
   * @return A TooltipInfo object containing the tooltip information.
   */
  virtual TooltipInfo getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(const std::vector<Id>& locationIds,
                                                                          const std::vector<Id>& localSymbolIds) const = 0;

  /**
   * @brief Set whether to use the error cache.
   * @param enabled True to enable the error cache, false to disable it.
   */
  virtual void setUseErrorCache([[maybe_unused]] bool enabled) {}

  /**
   * @brief Add errors to the cache.
   * @param newErrors A vector of ErrorInfo objects representing the new errors to add.
   * @param errorCount An ErrorCountInfo object containing the updated error count.
   */
  virtual void addErrorsToCache([[maybe_unused]] const std::vector<ErrorInfo>& newErrors, [[maybe_unused]] const ErrorCountInfo& errorCount) {}
};
