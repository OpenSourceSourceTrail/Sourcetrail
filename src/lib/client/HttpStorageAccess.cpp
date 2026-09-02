#include "HttpStorageAccess.h"

#include <optional>
#include <string>

#include "ConvertGraph.h"
#include "ConvertLocations.h"
#include "ConvertQuery.h"
#include "data/location/SourceLocationCollection.h"
#include "data/location/SourceLocationFile.h"
#include "data/name/NameHierarchy.h"
#include "engine.pb.h"
#include "EngineCall.h"
#include "EngineChannel.h"
#include "FileInfo.h"
#include "FilePath.h"
#include "graph/domain/Graph.h"
#include "graph/domain/NodeType.h"
#include "graph/domain/NodeTypeSet.h"
#include "logging.h"
#include "ProtoJson.h"
#include "TextAccess.h"
#include "utilityString.h"

namespace {

using client::call;
using client::callVoid;
using client::joinIds;
using client::urlEncode;

std::string toUtf8(const std::wstring& str) {
  return utility::encodeToUtf8(str);
}

std::string toUtf8(const FilePath& path) {
  return utility::encodeToUtf8(path.wstr());
}

std::vector<Id> toIds(const google::protobuf::RepeatedField<google::protobuf::uint64>& ids) {
  return {ids.begin(), ids.end()};
}

std::string toJson(const google::protobuf::Message& message) {
  return proto::json::toJson(message);
}

std::shared_ptr<Graph> graphFrom(const std::optional<sourcetrail::GraphQueryResponse>& response) {
  if(!response) {
    return std::make_shared<Graph>();
  }
  return proto::convert::fromProto(response->graph());
}

std::shared_ptr<SourceLocationCollection> collectionFrom(const std::optional<sourcetrail::SourceLocationCollectionResponse>& response) {
  if(!response) {
    return std::make_shared<SourceLocationCollection>();
  }
  return proto::convert::fromProto(*response);
}

std::vector<SearchMatch> matchesFrom(const std::optional<sourcetrail::SearchMatchesResponse>& response) {
  std::vector<SearchMatch> matches;
  if(!response) {
    return matches;
  }
  matches.reserve(static_cast<size_t>(response->matches_size()));
  for(const auto& match : response->matches()) {
    matches.push_back(proto::convert::fromProto(match));
  }
  return matches;
}

std::vector<ErrorInfo> errorsFrom(const std::optional<sourcetrail::ErrorInfosResponse>& response) {
  std::vector<ErrorInfo> errors;
  if(!response) {
    return errors;
  }
  errors.reserve(static_cast<size_t>(response->errors_size()));
  for(const auto& error : response->errors()) {
    errors.push_back(proto::convert::fromProto(error));
  }
  return errors;
}

sourcetrail::ProtoBookmark bookmarkBaseToProto(const Bookmark& bookmark) {
  sourcetrail::ProtoBookmark msg;
  msg.set_id(bookmark.getId());
  msg.set_name(toUtf8(bookmark.getName()));
  msg.set_comment(toUtf8(bookmark.getComment()));
  msg.set_timestamp(bookmark.getTimeStamp().toString());
  *msg.mutable_category() = proto::convert::toProto(bookmark.getCategory());
  return msg;
}

/** The query string the error endpoints share. */
std::string errorFilterQuery(const ErrorFilter& filter) {
  const sourcetrail::ProtoErrorFilter proto = proto::convert::toProto(filter);
  return std::string("error=") + (proto.error() ? "true" : "false") + "&fatal=" + (proto.fatal() ? "true" : "false") +
      "&unindexedError=" + (proto.unindexed_error() ? "true" : "false") +
      "&unindexedFatal=" + (proto.unindexed_fatal() ? "true" : "false") + "&limit=" + std::to_string(proto.limit());
}

/** One-off resolve batch, for the callers that still ask about a single symbol. */
std::optional<sourcetrail::SymbolResolveResponse> resolve(EngineChannel* channel,
                                                          const char* what,
                                                          const sourcetrail::SymbolResolveRequest& request) {
  return call<sourcetrail::SymbolResolveResponse>(channel, what, "POST", "/api/v1/symbols/resolve", toJson(request));
}

}    // namespace

HttpStorageAccess::HttpStorageAccess(EngineChannel* channel) : mChannel(channel) {}

// ---- Nodes and names -------------------------------------------------------

Id HttpStorageAccess::getNodeIdForFileNode(const FilePath& filePath) const {
  sourcetrail::SymbolResolveRequest request;
  request.add_file_paths(toUtf8(filePath));

  const auto response = resolve(mChannel, "getNodeIdForFileNode", request);
  return response && response->file_path_node_ids_size() > 0 ? response->file_path_node_ids(0) : 0;
}

Id HttpStorageAccess::getNodeIdForNameHierarchy(const NameHierarchy& nameHierarchy) const {
  sourcetrail::SymbolResolveRequest request;
  request.add_name_hierarchies(toUtf8(NameHierarchy::serialize(nameHierarchy)));

  const auto response = resolve(mChannel, "getNodeIdForNameHierarchy", request);
  return response && response->name_hierarchy_node_ids_size() > 0 ? response->name_hierarchy_node_ids(0) : 0;
}

std::vector<Id> HttpStorageAccess::getNodeIdsForNameHierarchies(const std::vector<NameHierarchy> nameHierarchies) const {
  sourcetrail::SymbolResolveRequest request;
  for(const NameHierarchy& nameHierarchy : nameHierarchies) {
    request.add_name_hierarchies(toUtf8(NameHierarchy::serialize(nameHierarchy)));
  }

  const auto response = resolve(mChannel, "getNodeIdsForNameHierarchies", request);
  return response ? toIds(response->name_hierarchy_node_ids()) : std::vector<Id>{};
}

NameHierarchy HttpStorageAccess::getNameHierarchyForNodeId(Id nodeId) const {
  sourcetrail::SymbolResolveRequest request;
  request.add_node_ids(nodeId);

  const auto response = resolve(mChannel, "getNameHierarchyForNodeId", request);
  if(!response || response->nodes_size() == 0) {
    return NameHierarchy(NAME_DELIMITER_UNKNOWN);
  }
  return NameHierarchy::deserialize(utility::decodeFromUtf8(response->nodes(0).name_hierarchy_serialized()));
}

std::vector<NameHierarchy> HttpStorageAccess::getNameHierarchiesForNodeIds(const std::vector<Id>& nodeIds) const {
  sourcetrail::SymbolResolveRequest request;
  for(const Id nodeId : nodeIds) {
    request.add_node_ids(nodeId);
  }

  const auto response = resolve(mChannel, "getNameHierarchiesForNodeIds", request);

  std::vector<NameHierarchy> hierarchies;
  if(!response) {
    return hierarchies;
  }
  hierarchies.reserve(static_cast<size_t>(response->nodes_size()));
  for(const auto& node : response->nodes()) {
    hierarchies.push_back(NameHierarchy::deserialize(utility::decodeFromUtf8(node.name_hierarchy_serialized())));
  }
  return hierarchies;
}

std::map<Id, std::pair<Id, NameHierarchy>> HttpStorageAccess::getNodeIdToParentFileMap(const std::vector<Id>& nodeIds) const {
  sourcetrail::SymbolResolveRequest request;
  for(const Id nodeId : nodeIds) {
    request.add_parent_file_node_ids(nodeId);
  }

  const auto response = resolve(mChannel, "getNodeIdToParentFileMap", request);

  std::map<Id, std::pair<Id, NameHierarchy>> result;
  if(!response) {
    return result;
  }
  for(const auto& entry : response->parent_files()) {
    result.emplace(entry.node_id(),
                   std::make_pair(entry.parent_file_id(),
                                  NameHierarchy::deserialize(utility::decodeFromUtf8(entry.name_hierarchy_serialized()))));
  }
  return result;
}

NodeType HttpStorageAccess::getNodeTypeForNodeWithId(Id nodeId) const {
  sourcetrail::SymbolResolveRequest request;
  request.add_node_ids(nodeId);
  request.set_include_node_kinds(true);

  const auto response = resolve(mChannel, "getNodeTypeForNodeWithId", request);
  if(!response || response->nodes_size() == 0) {
    return NodeType(NODE_SYMBOL);
  }
  return NodeType(intToNodeKind(response->nodes(0).node_kind()));
}

StorageEdge HttpStorageAccess::getEdgeById(Id edgeId) const {
  sourcetrail::SymbolResolveRequest request;
  request.add_edge_ids(edgeId);

  const auto response = resolve(mChannel, "getEdgeById", request);
  if(!response || response->edges_size() == 0) {
    return {};
  }
  const auto& edge = response->edges(0);
  return {edge.id(), edge.type(), edge.source_node_id(), edge.target_node_id()};
}

// ---- Search ----------------------------------------------------------------

std::shared_ptr<SourceLocationCollection> HttpStorageAccess::getFullTextSearchLocations(const std::wstring& searchTerm,
                                                                                        bool caseSensitive) const {
  const std::string target = "/api/v1/search/fulltext?q=" + urlEncode(toUtf8(searchTerm)) +
      "&case=" + (caseSensitive ? "true" : "false");
  return collectionFrom(call<sourcetrail::SourceLocationCollectionResponse>(mChannel, "getFullTextSearchLocations", "GET", target));
}

std::vector<SearchMatch> HttpStorageAccess::getAutocompletionMatches(const std::wstring& query,
                                                                     NodeTypeSet acceptedNodeTypes,
                                                                     bool acceptCommands) const {
  const std::string target = "/api/v1/search?q=" + urlEncode(toUtf8(query)) +
      "&types=" + std::to_string(proto::convert::toNodeKindMask(acceptedNodeTypes)) +
      "&commands=" + (acceptCommands ? "true" : "false");
  return matchesFrom(call<sourcetrail::SearchMatchesResponse>(mChannel, "getAutocompletionMatches", "GET", target));
}

std::vector<SearchMatch> HttpStorageAccess::getSearchMatchesForTokenIds(const std::vector<Id>& tokenIds) const {
  sourcetrail::SymbolResolveRequest request;
  for(const Id tokenId : tokenIds) {
    request.add_search_match_token_ids(tokenId);
  }

  const auto response = resolve(mChannel, "getSearchMatchesForTokenIds", request);

  std::vector<SearchMatch> matches;
  if(!response) {
    return matches;
  }
  matches.reserve(static_cast<size_t>(response->search_matches_size()));
  for(const auto& match : response->search_matches()) {
    matches.push_back(proto::convert::fromProto(match));
  }
  return matches;
}

// ---- Graphs ----------------------------------------------------------------

std::shared_ptr<Graph> HttpStorageAccess::getGraphForAll() const {
  return graphFrom(call<sourcetrail::GraphQueryResponse>(mChannel, "getGraphForAll", "GET", "/api/v1/graph?mode=all"));
}

std::shared_ptr<Graph> HttpStorageAccess::getGraphForNodeTypes(NodeTypeSet nodeTypes) const {
  const std::string target = "/api/v1/graph?mode=types&mask=" + std::to_string(proto::convert::toNodeKindMask(nodeTypes));
  return graphFrom(call<sourcetrail::GraphQueryResponse>(mChannel, "getGraphForNodeTypes", "GET", target));
}

std::shared_ptr<Graph> HttpStorageAccess::getGraphForActiveTokenIds(const std::vector<Id>& tokenIds,
                                                                    const std::vector<Id>& expandedNodeIds,
                                                                    bool* isActiveNamespace) const {
  const std::string target = "/api/v1/graph?mode=active&tokens=" + joinIds(tokenIds) + "&expanded=" + joinIds(expandedNodeIds);

  const auto response = call<sourcetrail::GraphQueryResponse>(mChannel, "getGraphForActiveTokenIds", "GET", target);
  if(isActiveNamespace != nullptr) {
    *isActiveNamespace = response && response->is_active_namespace();
  }
  return graphFrom(response);
}

std::shared_ptr<Graph> HttpStorageAccess::getGraphForChildrenOfNodeId(Id nodeId) const {
  const std::string target = "/api/v1/graph?mode=children&id=" + std::to_string(nodeId);
  return graphFrom(call<sourcetrail::GraphQueryResponse>(mChannel, "getGraphForChildrenOfNodeId", "GET", target));
}

std::shared_ptr<Graph> HttpStorageAccess::getGraphForTrail(Id originId,
                                                           Id targetId,
                                                           NodeKindMask nodeTypes,
                                                           Edge::TypeMask edgeTypes,
                                                           bool nodeNonIndexed,
                                                           size_t depth,
                                                           bool directed) const {
  const std::string target = "/api/v1/graph?mode=trail&origin=" + std::to_string(originId) +
      "&target=" + std::to_string(targetId) + "&nodeMask=" + std::to_string(nodeTypes) +
      "&edgeMask=" + std::to_string(edgeTypes) + "&nonIndexed=" + (nodeNonIndexed ? "true" : "false") +
      "&depth=" + std::to_string(depth) + "&directed=" + (directed ? "true" : "false");
  return graphFrom(call<sourcetrail::GraphQueryResponse>(mChannel, "getGraphForTrail", "GET", target));
}

NodeKindMask HttpStorageAccess::getAvailableNodeTypes() const {
  const auto response = call<sourcetrail::StatsResponse>(mChannel, "getAvailableNodeTypes", "GET", "/api/v1/stats?include=types");
  return response ? response->available_node_types() : 0;
}

Edge::TypeMask HttpStorageAccess::getAvailableEdgeTypes() const {
  const auto response = call<sourcetrail::StatsResponse>(mChannel, "getAvailableEdgeTypes", "GET", "/api/v1/stats?include=types");
  return response ? response->available_edge_types() : 0;
}

// ---- Source locations ------------------------------------------------------

std::vector<Id> HttpStorageAccess::getActiveTokenIdsForId(Id tokenId, Id* declarationId) const {
  const std::string target = "/api/v1/tokens/active?id=" + std::to_string(tokenId);

  const auto response = call<sourcetrail::ActiveTokenIdsResponse>(mChannel, "getActiveTokenIdsForId", "GET", target);
  if(declarationId != nullptr) {
    *declarationId = response ? response->declaration_id() : 0;
  }
  return response ? toIds(response->ids()) : std::vector<Id>{};
}

std::vector<Id> HttpStorageAccess::getNodeIdsForLocationIds(const std::vector<Id>& locationIds) const {
  sourcetrail::SymbolResolveRequest request;
  for(const Id locationId : locationIds) {
    request.add_location_ids(locationId);
  }

  const auto response = resolve(mChannel, "getNodeIdsForLocationIds", request);
  return response ? toIds(response->location_node_ids()) : std::vector<Id>{};
}

std::shared_ptr<SourceLocationCollection> HttpStorageAccess::getSourceLocationsForTokenIds(const std::vector<Id>& tokenIds) const {
  const std::string target = "/api/v1/locations?tokens=" + joinIds(tokenIds);
  return collectionFrom(
      call<sourcetrail::SourceLocationCollectionResponse>(mChannel, "getSourceLocationsForTokenIds", "GET", target));
}

std::shared_ptr<SourceLocationCollection> HttpStorageAccess::getSourceLocationsForLocationIds(const std::vector<Id>& locationIds) const {
  const std::string target = "/api/v1/locations?locations=" + joinIds(locationIds);
  return collectionFrom(
      call<sourcetrail::SourceLocationCollectionResponse>(mChannel, "getSourceLocationsForLocationIds", "GET", target));
}

std::shared_ptr<SourceLocationFile> HttpStorageAccess::getSourceLocationsForFile(const FilePath& filePath) const {
  const std::string target = "/api/v1/files/" + urlEncode(toUtf8(filePath)) + "?include=locations";

  const auto response = call<sourcetrail::FileResponse>(mChannel, "getSourceLocationsForFile", "GET", target);
  if(!response) {
    // Keep the requested path: the code view titles the file from it even when it has no locations.
    return std::make_shared<SourceLocationFile>(filePath, L"", true, false, false);
  }
  return proto::convert::fromProto(response->locations());
}

std::shared_ptr<SourceLocationFile> HttpStorageAccess::getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                                        size_t startLine,
                                                                                        size_t endLine) const {
  const std::string target = "/api/v1/files/" + urlEncode(toUtf8(filePath)) +
      "?include=locations&lines=" + std::to_string(startLine) + "," + std::to_string(endLine);

  const auto response = call<sourcetrail::FileResponse>(mChannel, "getSourceLocationsForLinesInFile", "GET", target);
  if(!response) {
    return std::make_shared<SourceLocationFile>(filePath, L"", false, false, false);
  }
  return proto::convert::fromProto(response->locations());
}

std::shared_ptr<SourceLocationFile> HttpStorageAccess::getSourceLocationsOfTypeInFile(const FilePath& filePath,
                                                                                      LocationType type) const {
  const std::string target = "/api/v1/files/" + urlEncode(toUtf8(filePath)) +
      "?include=locations&locationType=" + std::to_string(static_cast<int32_t>(type));

  const auto response = call<sourcetrail::FileResponse>(mChannel, "getSourceLocationsOfTypeInFile", "GET", target);
  if(!response) {
    return std::make_shared<SourceLocationFile>(filePath, L"", true, false, false);
  }
  return proto::convert::fromProto(response->locations());
}

// ---- Files -----------------------------------------------------------------

std::shared_ptr<TextAccess> HttpStorageAccess::getFileContent(const FilePath& filePath, bool showsErrors) const {
  const std::string target = "/api/v1/files/" + urlEncode(toUtf8(filePath)) +
      "?include=content&showsErrors=" + (showsErrors ? "true" : "false");

  const auto response = call<sourcetrail::FileResponse>(mChannel, "getFileContent", "GET", target);
  return TextAccess::createFromString(response ? response->content() : std::string(), filePath);
}

FileInfo HttpStorageAccess::getFileInfoForFileId(Id fileId) const {
  // The engine answers file info by path; a file node's name *is* its path, so one resolve gets it.
  sourcetrail::SymbolResolveRequest request;
  request.add_node_ids(fileId);

  const auto resolved = resolve(mChannel, "getFileInfoForFileId", request);
  if(!resolved || resolved->nodes_size() == 0) {
    return {};
  }

  const NameHierarchy name = NameHierarchy::deserialize(utility::decodeFromUtf8(resolved->nodes(0).name_hierarchy_serialized()));
  return getFileInfoForFilePath(FilePath(name.getQualifiedName()));
}

FileInfo HttpStorageAccess::getFileInfoForFilePath(const FilePath& filePath) const {
  const std::string target = "/api/v1/files/" + urlEncode(toUtf8(filePath)) + "?include=info";

  const auto response = call<sourcetrail::FileResponse>(mChannel, "getFileInfoForFilePath", "GET", target);
  return response ? proto::convert::fromProto(response->info()) : FileInfo{};
}

std::vector<FileInfo> HttpStorageAccess::getFileInfosForFilePaths(const std::vector<FilePath>& filePaths) const {
  sourcetrail::FilePathsRequest request;
  for(const FilePath& filePath : filePaths) {
    request.add_file_paths(toUtf8(filePath));
  }

  const auto response = call<sourcetrail::FileInfosResponse>(
      mChannel, "getFileInfosForFilePaths", "POST", "/api/v1/files/info", toJson(request));

  std::vector<FileInfo> infos;
  if(!response) {
    return infos;
  }
  infos.reserve(static_cast<size_t>(response->file_infos_size()));
  for(const auto& info : response->file_infos()) {
    infos.push_back(proto::convert::fromProto(info));
  }
  return infos;
}

StorageStats HttpStorageAccess::getStorageStats() const {
  const auto response = call<sourcetrail::StatsResponse>(mChannel, "getStorageStats", "GET", "/api/v1/stats?include=counts");
  return response ? proto::convert::fromProto(response->stats()) : StorageStats{};
}

// ---- Errors ----------------------------------------------------------------

ErrorCountInfo HttpStorageAccess::getErrorCount() const {
  const auto response = call<sourcetrail::StatsResponse>(mChannel, "getErrorCount", "GET", "/api/v1/stats?include=errors");
  if(!response) {
    return {};
  }
  return {response->error_total(), response->error_fatal()};
}

std::vector<ErrorInfo> HttpStorageAccess::getErrorsLimited(const ErrorFilter& filter) const {
  const std::string target = "/api/v1/errors?" + errorFilterQuery(filter);
  return errorsFrom(call<sourcetrail::ErrorInfosResponse>(mChannel, "getErrorsLimited", "GET", target));
}

std::vector<ErrorInfo> HttpStorageAccess::getErrorsForFileLimited(const ErrorFilter& filter, const FilePath& filePath) const {
  const std::string target = "/api/v1/errors?" + errorFilterQuery(filter) + "&file=" + urlEncode(toUtf8(filePath));
  return errorsFrom(call<sourcetrail::ErrorInfosResponse>(mChannel, "getErrorsForFileLimited", "GET", target));
}

std::shared_ptr<SourceLocationCollection> HttpStorageAccess::getErrorSourceLocations(const std::vector<ErrorInfo>& errors) const {
  // Only the fields the engine actually reads are sent. The messages -- by far the bulk of an error
  // list -- stay here, where the caller already has them.
  sourcetrail::ErrorLocationsRequest request;
  for(const ErrorInfo& error : errors) {
    auto* entry = request.add_errors();
    entry->set_id(error.id);
    entry->set_file_path(toUtf8(error.filePath));
    entry->set_line_number(error.lineNumber);
    entry->set_column_number(error.columnNumber);
  }

  return collectionFrom(call<sourcetrail::SourceLocationCollectionResponse>(
      mChannel, "getErrorSourceLocations", "POST", "/api/v1/errors/locations", toJson(request)));
}

// ---- Bookmarks -------------------------------------------------------------

Id HttpStorageAccess::addNodeBookmark(const NodeBookmark& bookmark) {
  sourcetrail::AddBookmarkRequest request;
  *request.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id nodeId : bookmark.getNodeIds()) {
    request.add_node_ids(nodeId);
  }

  const auto response = call<sourcetrail::BookmarkIdResponse>(
      mChannel, "addNodeBookmark", "POST", "/api/v1/bookmarks", toJson(request));
  return response ? response->id() : 0;
}

Id HttpStorageAccess::addEdgeBookmark(const EdgeBookmark& bookmark) {
  sourcetrail::AddBookmarkRequest request;
  *request.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id edgeId : bookmark.getEdgeIds()) {
    request.add_edge_ids(edgeId);
  }
  request.set_active_node_id(bookmark.getActiveNodeId());

  const auto response = call<sourcetrail::BookmarkIdResponse>(
      mChannel, "addEdgeBookmark", "POST", "/api/v1/bookmarks", toJson(request));
  return response ? response->id() : 0;
}

Id HttpStorageAccess::addBookmarkCategory(const std::wstring& categoryName) {
  // Categories are created implicitly by updateBookmark's category name; no GUI path reaches here.
  LOG_WARNING("addBookmarkCategory is not exposed over the engine boundary: " + toUtf8(categoryName));
  return 0;
}

void HttpStorageAccess::updateBookmark(const Id bookmarkId,
                                       const std::wstring& name,
                                       const std::wstring& comment,
                                       const std::wstring& categoryName) {
  sourcetrail::UpdateBookmarkRequest request;
  request.set_bookmark_id(bookmarkId);
  request.set_name(toUtf8(name));
  request.set_comment(toUtf8(comment));
  request.set_category_name(toUtf8(categoryName));

  callVoid(mChannel, "updateBookmark", "PATCH", "/api/v1/bookmarks/" + std::to_string(bookmarkId), toJson(request));
}

void HttpStorageAccess::removeBookmark(const Id bookmarkId) {
  callVoid(mChannel, "removeBookmark", "DELETE", "/api/v1/bookmarks/" + std::to_string(bookmarkId));
}

void HttpStorageAccess::removeBookmarkCategory(const Id bookmarkCategoryId) {
  callVoid(mChannel, "removeBookmarkCategory", "DELETE", "/api/v1/bookmarks/categories/" + std::to_string(bookmarkCategoryId));
}

std::vector<NodeBookmark> HttpStorageAccess::getAllNodeBookmarks() const {
  const auto response = call<sourcetrail::BookmarksResponse>(mChannel, "getAllNodeBookmarks", "GET", "/api/v1/bookmarks");

  std::vector<NodeBookmark> bookmarks;
  if(!response) {
    return bookmarks;
  }
  bookmarks.reserve(static_cast<size_t>(response->node_bookmarks_size()));
  for(const auto& bookmark : response->node_bookmarks()) {
    bookmarks.push_back(proto::convert::fromProto(bookmark));
  }
  return bookmarks;
}

std::vector<EdgeBookmark> HttpStorageAccess::getAllEdgeBookmarks() const {
  const auto response = call<sourcetrail::BookmarksResponse>(mChannel, "getAllEdgeBookmarks", "GET", "/api/v1/bookmarks");

  std::vector<EdgeBookmark> bookmarks;
  if(!response) {
    return bookmarks;
  }
  bookmarks.reserve(static_cast<size_t>(response->edge_bookmarks_size()));
  for(const auto& bookmark : response->edge_bookmarks()) {
    bookmarks.push_back(proto::convert::fromProto(bookmark));
  }
  return bookmarks;
}

std::vector<BookmarkCategory> HttpStorageAccess::getAllBookmarkCategories() const {
  const auto response = call<sourcetrail::BookmarksResponse>(mChannel, "getAllBookmarkCategories", "GET", "/api/v1/bookmarks");

  std::vector<BookmarkCategory> categories;
  if(!response) {
    return categories;
  }
  categories.reserve(static_cast<size_t>(response->categories_size()));
  for(const auto& category : response->categories()) {
    categories.push_back(proto::convert::fromProto(category));
  }
  return categories;
}

// ---- Tooltips --------------------------------------------------------------

TooltipInfo HttpStorageAccess::getTooltipInfoForTokenIds(const std::vector<Id>& tokenIds, TooltipOrigin origin) const {
  const std::string target = "/api/v1/tooltip?tokens=" + joinIds(tokenIds) +
      "&origin=" + std::to_string(static_cast<int32_t>(origin));

  const auto response = call<sourcetrail::TooltipInfoResponse>(mChannel, "getTooltipInfoForTokenIds", "GET", target);
  return response ? proto::convert::fromProto(response->info()) : TooltipInfo{};
}

TooltipInfo HttpStorageAccess::getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(const std::vector<Id>& locationIds,
                                                                                   const std::vector<Id>& localSymbolIds) const {
  const std::string target = "/api/v1/tooltip?locations=" + joinIds(locationIds) + "&locals=" + joinIds(localSymbolIds);

  const auto response = call<sourcetrail::TooltipInfoResponse>(
      mChannel, "getTooltipInfoForSourceLocationIdsAndLocalSymbolIds", "GET", target);
  return response ? proto::convert::fromProto(response->info()) : TooltipInfo{};
}
