#include "GrpcStorageAccess.h"

#include <optional>

#include <grpcpp/grpcpp.h>

#include "ConvertGraph.h"
#include "ConvertLocations.h"
#include "ConvertQuery.h"
#include "EngineChannel.h"
#include "FileInfo.h"
#include "FilePath.h"
#include "Graph.h"
#include "NameHierarchy.h"
#include "NodeType.h"
#include "NodeTypeSet.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"
#include "TextAccess.h"
#include "engine.grpc.pb.h"
#include "logging.h"
#include "utilityString.h"

namespace {

using Stub = sourcetrail::EngineService::Stub;

template <class Request, class Response>
using UnaryRpc = grpc::Status (Stub::*)(grpc::ClientContext*, const Request&, Response*);

/**
 * The one place an engine failure becomes an empty answer.
 *
 * Returns nullopt when the engine is unreachable, misses the deadline, or answers non-OK. The
 * deadline matters as much as the error handling: a UI thread blocked on a hung engine is
 * indistinguishable from a frozen application.
 */
template <class Request, class Response>
std::optional<Response> call(EngineChannel* channel, const char* rpcName, UnaryRpc<Request, Response> method,
                             const Request& request) {
  if(channel == nullptr || channel->getStub() == nullptr) {
    return std::nullopt;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + channel->getCallTimeout());

  Response response;
  const grpc::Status status = (channel->getStub()->*method)(&context, request, &response);
  if(!status.ok()) {
    LOG_WARNING(std::string("Engine call ") + rpcName + " failed: " + status.error_message());
    channel->markDegraded();
    return std::nullopt;
  }

  channel->markConnected();
  return response;
}

std::string toUtf8(const std::wstring& str) {
  return utility::encodeToUtf8(str);
}

std::string toUtf8(const FilePath& path) {
  return utility::encodeToUtf8(path.wstr());
}

sourcetrail::IdsRequest makeIdsRequest(const std::vector<Id>& ids) {
  sourcetrail::IdsRequest request;
  for(const Id id : ids) {
    request.add_ids(id);
  }
  return request;
}

sourcetrail::IdRequest makeIdRequest(Id id) {
  sourcetrail::IdRequest request;
  request.set_id(id);
  return request;
}

sourcetrail::FilePathRequest makeFilePathRequest(const FilePath& path) {
  sourcetrail::FilePathRequest request;
  request.set_file_path(toUtf8(path));
  return request;
}

std::vector<Id> toIds(const google::protobuf::RepeatedField<google::protobuf::uint64>& ids) {
  return {ids.begin(), ids.end()};
}

std::shared_ptr<Graph> graphFrom(const std::optional<sourcetrail::GraphResponse>& response) {
  if(!response) {
    return std::make_shared<Graph>();
  }
  return proto::convert::fromProto(response->graph());
}

std::shared_ptr<SourceLocationCollection> collectionFrom(
    const std::optional<sourcetrail::SourceLocationCollectionResponse>& response) {
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

}    // namespace

GrpcStorageAccess::GrpcStorageAccess(EngineChannel* channel) : mChannel(channel) {}

// ---- Nodes and names -------------------------------------------------------

Id GrpcStorageAccess::getNodeIdForFileNode(const FilePath& filePath) const {
  const auto response = call(mChannel, "GetNodeIdForFileNode", &Stub::GetNodeIdForFileNode, makeFilePathRequest(filePath));
  return response ? response->id() : 0;
}

Id GrpcStorageAccess::getNodeIdForNameHierarchy(const NameHierarchy& nameHierarchy) const {
  sourcetrail::NameHierarchyRequest request;
  request.set_serialized(toUtf8(NameHierarchy::serialize(nameHierarchy)));

  const auto response = call(mChannel, "GetNodeIdForNameHierarchy", &Stub::GetNodeIdForNameHierarchy, request);
  return response ? response->id() : 0;
}

std::vector<Id> GrpcStorageAccess::getNodeIdsForNameHierarchies(const std::vector<NameHierarchy> nameHierarchies) const {
  sourcetrail::NameHierarchiesRequest request;
  for(const NameHierarchy& nameHierarchy : nameHierarchies) {
    request.add_serialized(toUtf8(NameHierarchy::serialize(nameHierarchy)));
  }

  const auto response = call(mChannel, "GetNodeIdsForNameHierarchies", &Stub::GetNodeIdsForNameHierarchies, request);
  return response ? toIds(response->ids()) : std::vector<Id>{};
}

NameHierarchy GrpcStorageAccess::getNameHierarchyForNodeId(Id nodeId) const {
  const auto response = call(mChannel, "GetNameHierarchyForNodeId", &Stub::GetNameHierarchyForNodeId, makeIdRequest(nodeId));
  if(!response) {
    return NameHierarchy(NAME_DELIMITER_UNKNOWN);
  }
  return NameHierarchy::deserialize(utility::decodeFromUtf8(response->serialized()));
}

std::vector<NameHierarchy> GrpcStorageAccess::getNameHierarchiesForNodeIds(const std::vector<Id>& nodeIds) const {
  const auto response = call(
      mChannel, "GetNameHierarchiesForNodeIds", &Stub::GetNameHierarchiesForNodeIds, makeIdsRequest(nodeIds));

  std::vector<NameHierarchy> hierarchies;
  if(!response) {
    return hierarchies;
  }
  hierarchies.reserve(static_cast<size_t>(response->serialized_size()));
  for(const auto& serialized : response->serialized()) {
    hierarchies.push_back(NameHierarchy::deserialize(utility::decodeFromUtf8(serialized)));
  }
  return hierarchies;
}

std::map<Id, std::pair<Id, NameHierarchy>> GrpcStorageAccess::getNodeIdToParentFileMap(
    const std::vector<Id>& nodeIds) const {
  const auto response = call(
      mChannel, "GetNodeIdToParentFileMap", &Stub::GetNodeIdToParentFileMap, makeIdsRequest(nodeIds));

  std::map<Id, std::pair<Id, NameHierarchy>> result;
  if(!response) {
    return result;
  }
  for(const auto& entry : response->entries()) {
    result.emplace(entry.node_id(),
                   std::make_pair(entry.parent_file_id(),
                                  NameHierarchy::deserialize(utility::decodeFromUtf8(entry.name_hierarchy_serialized()))));
  }
  return result;
}

NodeType GrpcStorageAccess::getNodeTypeForNodeWithId(Id nodeId) const {
  const auto response = call(
      mChannel, "GetNodeTypeForNodeWithId", &Stub::GetNodeTypeForNodeWithId, makeIdRequest(nodeId));
  return NodeType(response ? intToNodeKind(response->kind()) : NODE_SYMBOL);
}

StorageEdge GrpcStorageAccess::getEdgeById(Id edgeId) const {
  const auto response = call(mChannel, "GetEdgeById", &Stub::GetEdgeById, makeIdRequest(edgeId));
  if(!response) {
    return {};
  }
  const auto& edge = response->edge();
  return {edge.id(), edge.type(), edge.source_node_id(), edge.target_node_id()};
}

// ---- Search ----------------------------------------------------------------

std::shared_ptr<SourceLocationCollection> GrpcStorageAccess::getFullTextSearchLocations(const std::wstring& searchTerm,
                                                                                        bool caseSensitive) const {
  sourcetrail::FullTextSearchRequest request;
  request.set_search_term(toUtf8(searchTerm));
  request.set_case_sensitive(caseSensitive);

  return collectionFrom(call(mChannel, "GetFullTextSearchLocations", &Stub::GetFullTextSearchLocations, request));
}

std::vector<SearchMatch> GrpcStorageAccess::getAutocompletionMatches(const std::wstring& query,
                                                                      NodeTypeSet acceptedNodeTypes,
                                                                      bool acceptCommands) const {
  sourcetrail::AutocompletionRequest request;
  request.set_query(toUtf8(query));
  request.set_accepted_node_types_mask(proto::convert::toNodeKindMask(acceptedNodeTypes));
  request.set_accept_commands(acceptCommands);

  return matchesFrom(call(mChannel, "GetAutocompletionMatches", &Stub::GetAutocompletionMatches, request));
}

std::vector<SearchMatch> GrpcStorageAccess::getSearchMatchesForTokenIds(const std::vector<Id>& tokenIds) const {
  return matchesFrom(
      call(mChannel, "GetSearchMatchesForTokenIds", &Stub::GetSearchMatchesForTokenIds, makeIdsRequest(tokenIds)));
}

// ---- Graphs ----------------------------------------------------------------

std::shared_ptr<Graph> GrpcStorageAccess::getGraphForAll() const {
  return graphFrom(call(mChannel, "GetGraphForAll", &Stub::GetGraphForAll, sourcetrail::EmptyRequest{}));
}

std::shared_ptr<Graph> GrpcStorageAccess::getGraphForNodeTypes(NodeTypeSet nodeTypes) const {
  sourcetrail::NodeKindMaskRequest request;
  request.set_mask(proto::convert::toNodeKindMask(nodeTypes));

  return graphFrom(call(mChannel, "GetGraphForNodeTypes", &Stub::GetGraphForNodeTypes, request));
}

std::shared_ptr<Graph> GrpcStorageAccess::getGraphForActiveTokenIds(const std::vector<Id>& tokenIds,
                                                                     const std::vector<Id>& expandedNodeIds,
                                                                     bool* isActiveNamespace) const {
  sourcetrail::ActiveTokensRequest request;
  for(const Id tokenId : tokenIds) {
    request.add_token_ids(tokenId);
  }
  for(const Id nodeId : expandedNodeIds) {
    request.add_expanded_node_ids(nodeId);
  }

  const auto response = call(mChannel, "GetGraphForActiveTokenIds", &Stub::GetGraphForActiveTokenIds, request);
  if(isActiveNamespace != nullptr) {
    *isActiveNamespace = response && response->is_active_namespace();
  }
  if(!response) {
    return std::make_shared<Graph>();
  }
  return proto::convert::fromProto(response->graph());
}

std::shared_ptr<Graph> GrpcStorageAccess::getGraphForChildrenOfNodeId(Id nodeId) const {
  return graphFrom(
      call(mChannel, "GetGraphForChildrenOfNodeId", &Stub::GetGraphForChildrenOfNodeId, makeIdRequest(nodeId)));
}

std::shared_ptr<Graph> GrpcStorageAccess::getGraphForTrail(Id originId,
                                                            Id targetId,
                                                            NodeKindMask nodeTypes,
                                                            Edge::TypeMask edgeTypes,
                                                            bool nodeNonIndexed,
                                                            size_t depth,
                                                            bool directed) const {
  sourcetrail::TrailRequest request;
  request.set_origin_id(originId);
  request.set_target_id(targetId);
  request.set_node_types_mask(nodeTypes);
  request.set_edge_types_mask(edgeTypes);
  request.set_node_non_indexed(nodeNonIndexed);
  request.set_depth(depth);
  request.set_directed(directed);

  return graphFrom(call(mChannel, "GetGraphForTrail", &Stub::GetGraphForTrail, request));
}

NodeKindMask GrpcStorageAccess::getAvailableNodeTypes() const {
  const auto response = call(mChannel, "GetAvailableNodeTypes", &Stub::GetAvailableNodeTypes, sourcetrail::EmptyRequest{});
  return response ? response->mask() : 0;
}

Edge::TypeMask GrpcStorageAccess::getAvailableEdgeTypes() const {
  const auto response = call(mChannel, "GetAvailableEdgeTypes", &Stub::GetAvailableEdgeTypes, sourcetrail::EmptyRequest{});
  return response ? response->mask() : 0;
}

// ---- Source locations ------------------------------------------------------

std::vector<Id> GrpcStorageAccess::getActiveTokenIdsForId(Id tokenId, Id* declarationId) const {
  sourcetrail::ActiveTokenIdRequest request;
  request.set_token_id(tokenId);

  const auto response = call(mChannel, "GetActiveTokenIdsForId", &Stub::GetActiveTokenIdsForId, request);
  if(declarationId != nullptr) {
    *declarationId = response ? response->declaration_id() : 0;
  }
  return response ? toIds(response->ids()) : std::vector<Id>{};
}

std::vector<Id> GrpcStorageAccess::getNodeIdsForLocationIds(const std::vector<Id>& locationIds) const {
  const auto response = call(
      mChannel, "GetNodeIdsForLocationIds", &Stub::GetNodeIdsForLocationIds, makeIdsRequest(locationIds));
  return response ? toIds(response->ids()) : std::vector<Id>{};
}

std::shared_ptr<SourceLocationCollection> GrpcStorageAccess::getSourceLocationsForTokenIds(
    const std::vector<Id>& tokenIds) const {
  return collectionFrom(
      call(mChannel, "GetSourceLocationsForTokenIds", &Stub::GetSourceLocationsForTokenIds, makeIdsRequest(tokenIds)));
}

std::shared_ptr<SourceLocationCollection> GrpcStorageAccess::getSourceLocationsForLocationIds(
    const std::vector<Id>& locationIds) const {
  return collectionFrom(call(
      mChannel, "GetSourceLocationsForLocationIds", &Stub::GetSourceLocationsForLocationIds, makeIdsRequest(locationIds)));
}

std::shared_ptr<SourceLocationFile> GrpcStorageAccess::getSourceLocationsForFile(const FilePath& filePath) const {
  const auto response = call(
      mChannel, "GetSourceLocationsForFile", &Stub::GetSourceLocationsForFile, makeFilePathRequest(filePath));
  if(!response) {
    // Keep the requested path: the code view titles the file from it even when it has no locations.
    return std::make_shared<SourceLocationFile>(filePath, L"", true, false, false);
  }
  return proto::convert::fromProto(response->file());
}

std::shared_ptr<SourceLocationFile> GrpcStorageAccess::getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                                        size_t startLine,
                                                                                        size_t endLine) const {
  sourcetrail::LinesInFileRequest request;
  request.set_file_path(toUtf8(filePath));
  request.set_start_line(startLine);
  request.set_end_line(endLine);

  const auto response = call(mChannel, "GetSourceLocationsForLinesInFile", &Stub::GetSourceLocationsForLinesInFile, request);
  if(!response) {
    return std::make_shared<SourceLocationFile>(filePath, L"", false, false, false);
  }
  return proto::convert::fromProto(response->file());
}

std::shared_ptr<SourceLocationFile> GrpcStorageAccess::getSourceLocationsOfTypeInFile(const FilePath& filePath,
                                                                                       LocationType type) const {
  sourcetrail::LocationTypeInFileRequest request;
  request.set_file_path(toUtf8(filePath));
  request.set_location_type(static_cast<int32_t>(type));

  const auto response = call(mChannel, "GetSourceLocationsOfTypeInFile", &Stub::GetSourceLocationsOfTypeInFile, request);
  if(!response) {
    return std::make_shared<SourceLocationFile>(filePath, L"", true, false, false);
  }
  return proto::convert::fromProto(response->file());
}

// ---- Files -----------------------------------------------------------------

std::shared_ptr<TextAccess> GrpcStorageAccess::getFileContent(const FilePath& filePath, bool showsErrors) const {
  sourcetrail::FileContentRequest request;
  request.set_file_path(toUtf8(filePath));
  request.set_shows_errors(showsErrors);

  const auto response = call(mChannel, "GetFileContent", &Stub::GetFileContent, request);
  return TextAccess::createFromString(response ? response->content() : std::string(), filePath);
}

FileInfo GrpcStorageAccess::getFileInfoForFileId(Id fileId) const {
  const auto response = call(mChannel, "GetFileInfoForFileId", &Stub::GetFileInfoForFileId, makeIdRequest(fileId));
  return response ? proto::convert::fromProto(response->file_info()) : FileInfo{};
}

FileInfo GrpcStorageAccess::getFileInfoForFilePath(const FilePath& filePath) const {
  const auto response = call(
      mChannel, "GetFileInfoForFilePath", &Stub::GetFileInfoForFilePath, makeFilePathRequest(filePath));
  return response ? proto::convert::fromProto(response->file_info()) : FileInfo{};
}

std::vector<FileInfo> GrpcStorageAccess::getFileInfosForFilePaths(const std::vector<FilePath>& filePaths) const {
  sourcetrail::FilePathsRequest request;
  for(const FilePath& filePath : filePaths) {
    request.add_file_paths(toUtf8(filePath));
  }

  const auto response = call(mChannel, "GetFileInfosForFilePaths", &Stub::GetFileInfosForFilePaths, request);

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

StorageStats GrpcStorageAccess::getStorageStats() const {
  const auto response = call(mChannel, "GetStorageStats", &Stub::GetStorageStats, sourcetrail::EmptyRequest{});
  return response ? proto::convert::fromProto(response->stats()) : StorageStats{};
}

// ---- Errors ----------------------------------------------------------------

ErrorCountInfo GrpcStorageAccess::getErrorCount() const {
  const auto response = call(mChannel, "GetErrorCount", &Stub::GetErrorCount, sourcetrail::EmptyRequest{});
  if(!response) {
    return {};
  }
  return {response->total(), response->fatal()};
}

std::vector<ErrorInfo> GrpcStorageAccess::getErrorsLimited(const ErrorFilter& filter) const {
  sourcetrail::ErrorFilterRequest request;
  *request.mutable_filter() = proto::convert::toProto(filter);

  return errorsFrom(call(mChannel, "GetErrorsLimited", &Stub::GetErrorsLimited, request));
}

std::vector<ErrorInfo> GrpcStorageAccess::getErrorsForFileLimited(const ErrorFilter& filter,
                                                                   const FilePath& filePath) const {
  sourcetrail::ErrorFilterFileRequest request;
  *request.mutable_filter() = proto::convert::toProto(filter);
  request.set_file_path(toUtf8(filePath));

  return errorsFrom(call(mChannel, "GetErrorsForFileLimited", &Stub::GetErrorsForFileLimited, request));
}

std::shared_ptr<SourceLocationCollection> GrpcStorageAccess::getErrorSourceLocations(
    const std::vector<ErrorInfo>& errors) const {
  sourcetrail::ErrorInfosRequest request;
  for(const ErrorInfo& error : errors) {
    *request.add_errors() = proto::convert::toProto(error);
  }

  return collectionFrom(call(mChannel, "GetErrorSourceLocations", &Stub::GetErrorSourceLocations, request));
}

// ---- Bookmarks -------------------------------------------------------------

Id GrpcStorageAccess::addNodeBookmark(const NodeBookmark& bookmark) {
  sourcetrail::AddNodeBookmarkRequest request;
  *request.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id nodeId : bookmark.getNodeIds()) {
    request.add_node_ids(nodeId);
  }

  const auto response = call(mChannel, "AddNodeBookmark", &Stub::AddNodeBookmark, request);
  return response ? response->id() : 0;
}

Id GrpcStorageAccess::addEdgeBookmark(const EdgeBookmark& bookmark) {
  sourcetrail::AddEdgeBookmarkRequest request;
  *request.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id edgeId : bookmark.getEdgeIds()) {
    request.add_edge_ids(edgeId);
  }
  request.set_active_node_id(bookmark.getActiveNodeId());

  const auto response = call(mChannel, "AddEdgeBookmark", &Stub::AddEdgeBookmark, request);
  return response ? response->id() : 0;
}

Id GrpcStorageAccess::addBookmarkCategory(const std::wstring& categoryName) {
  sourcetrail::AddBookmarkCategoryRequest request;
  request.set_name(toUtf8(categoryName));

  const auto response = call(mChannel, "AddBookmarkCategory", &Stub::AddBookmarkCategory, request);
  return response ? response->id() : 0;
}

void GrpcStorageAccess::updateBookmark(const Id bookmarkId,
                                        const std::wstring& name,
                                        const std::wstring& comment,
                                        const std::wstring& categoryName) {
  sourcetrail::UpdateBookmarkRequest request;
  request.set_bookmark_id(bookmarkId);
  request.set_name(toUtf8(name));
  request.set_comment(toUtf8(comment));
  request.set_category_name(toUtf8(categoryName));

  call(mChannel, "UpdateBookmark", &Stub::UpdateBookmark, request);
}

void GrpcStorageAccess::removeBookmark(const Id bookmarkId) {
  sourcetrail::RemoveBookmarkRequest request;
  request.set_bookmark_id(bookmarkId);

  call(mChannel, "RemoveBookmark", &Stub::RemoveBookmark, request);
}

void GrpcStorageAccess::removeBookmarkCategory(const Id bookmarkCategoryId) {
  sourcetrail::RemoveBookmarkCategoryRequest request;
  request.set_category_id(bookmarkCategoryId);

  call(mChannel, "RemoveBookmarkCategory", &Stub::RemoveBookmarkCategory, request);
}

std::vector<NodeBookmark> GrpcStorageAccess::getAllNodeBookmarks() const {
  const auto response = call(mChannel, "GetAllNodeBookmarks", &Stub::GetAllNodeBookmarks, sourcetrail::EmptyRequest{});

  std::vector<NodeBookmark> bookmarks;
  if(!response) {
    return bookmarks;
  }
  bookmarks.reserve(static_cast<size_t>(response->bookmarks_size()));
  for(const auto& bookmark : response->bookmarks()) {
    bookmarks.push_back(proto::convert::fromProto(bookmark));
  }
  return bookmarks;
}

std::vector<EdgeBookmark> GrpcStorageAccess::getAllEdgeBookmarks() const {
  const auto response = call(mChannel, "GetAllEdgeBookmarks", &Stub::GetAllEdgeBookmarks, sourcetrail::EmptyRequest{});

  std::vector<EdgeBookmark> bookmarks;
  if(!response) {
    return bookmarks;
  }
  bookmarks.reserve(static_cast<size_t>(response->bookmarks_size()));
  for(const auto& bookmark : response->bookmarks()) {
    bookmarks.push_back(proto::convert::fromProto(bookmark));
  }
  return bookmarks;
}

std::vector<BookmarkCategory> GrpcStorageAccess::getAllBookmarkCategories() const {
  const auto response = call(
      mChannel, "GetAllBookmarkCategories", &Stub::GetAllBookmarkCategories, sourcetrail::EmptyRequest{});

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

TooltipInfo GrpcStorageAccess::getTooltipInfoForTokenIds(const std::vector<Id>& tokenIds, TooltipOrigin origin) const {
  sourcetrail::TooltipTokenIdsRequest request;
  for(const Id tokenId : tokenIds) {
    request.add_token_ids(tokenId);
  }
  request.set_origin(static_cast<int32_t>(origin));

  const auto response = call(mChannel, "GetTooltipInfoForTokenIds", &Stub::GetTooltipInfoForTokenIds, request);
  return response ? proto::convert::fromProto(response->info()) : TooltipInfo{};
}

TooltipInfo GrpcStorageAccess::getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(
    const std::vector<Id>& locationIds, const std::vector<Id>& localSymbolIds) const {
  sourcetrail::TooltipLocationRequest request;
  for(const Id locationId : locationIds) {
    request.add_location_ids(locationId);
  }
  for(const Id symbolId : localSymbolIds) {
    request.add_local_symbol_ids(symbolId);
  }

  const auto response = call(mChannel,
                             "GetTooltipInfoForSourceLocationIdsAndLocalSymbolIds",
                             &Stub::GetTooltipInfoForSourceLocationIdsAndLocalSymbolIds,
                             request);
  return response ? proto::convert::fromProto(response->info()) : TooltipInfo{};
}
