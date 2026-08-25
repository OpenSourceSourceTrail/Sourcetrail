#include "EngineServiceImpl.h"

#include <grpcpp/grpcpp.h>

#include "Application.h"
#include "ConvertGraph.h"
#include "ConvertLocations.h"
#include "ConvertQuery.h"
#include "EdgeBookmark.h"
#include "ErrorFilter.h"
#include "ErrorInfo.h"
#include "FileInfo.h"
#include "FilePath.h"
#include "IndexerPluginRegistry.h"
#include "language_packages.h"
#include "logging.h"
#include "NameHierarchy.h"
#include "NodeBookmark.h"
#include "NodeKind.h"
#include "NodeType.h"
#include "NodeTypeSet.h"
#include "SearchMatch.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"
#include "StorageAccess.h"
#include "TextAccess.h"
#include "TooltipInfo.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageRefresh.h"
#include "utilityString.h"
#include "Version.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "CompilationDatabase.h"
#  include "utilitySourceGroupCxx.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

EngineServiceImpl::EngineServiceImpl(StorageAccess* storageAccess) : mStorageAccess(storageAccess) {}

// ---- Lifecycle -------------------------------------------------------------

grpc::Status EngineServiceImpl::LoadProject(grpc::ServerContext* /*ctx*/,
                                            const sourcetrail::LoadProjectRequest* req,
                                            sourcetrail::LoadProjectResponse* resp) {
  // Synchronous on purpose: the client is blocked on this response and has no other way to learn
  // whether the project opened. setSendAsTask(false) is what makes dispatchImmediately immediate --
  // MessageQueue::processMessage still hands the message to the task queue otherwise.
  MessageLoadProject message{FilePath(utility::decodeFromUtf8(req->project_file_path())), false};
  message.setSendAsTask(false);
  message.dispatchImmediately();

  const std::shared_ptr<IProject> project = Application::getInstance()->getCurrentProject();
  if(!project) {
    resp->set_error_message("Project could not be loaded.");
    return grpc::Status::OK;
  }

  resp->set_loaded(project->isLoaded());
  resp->set_indexing(project->isIndexing());
  resp->set_reindexable(project->isReindexable());
  resp->set_description(project->getDescription());
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::Refresh(grpc::ServerContext* /*ctx*/,
                                        const sourcetrail::RefreshRequest* req,
                                        sourcetrail::RefreshResponse* /*resp*/) {
  MessageRefresh msg;
  if(req->all()) {
    msg.refreshAll();
  }
  msg.dispatch();
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::IndexingInterrupted(grpc::ServerContext* /*ctx*/,
                                                    const sourcetrail::IndexingInterruptedRequest* /*req*/,
                                                    sourcetrail::IndexingInterruptedResponse* /*resp*/) {
  MessageIndexingInterrupted{}.dispatch();
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetCapabilities(grpc::ServerContext* /*ctx*/,
                                                const sourcetrail::EmptyRequest* /*req*/,
                                                sourcetrail::CapabilitiesResponse* resp) {
  const IndexerPluginRegistry::Ptr registry = IndexerPluginRegistry::getInstance();

  for(const IndexerPluginRegistry::Plugin& plugin : registry->getPlugins()) {
    sourcetrail::PluginInfo* info = resp->add_plugins();
    info->set_id(plugin.id);
    info->set_name(plugin.name);
    info->set_language(plugin.language);
    info->set_command_type(plugin.commandType);
    for(const SourceGroupType type : plugin.sourceGroupTypes) {
      info->add_source_group_types(type);
    }
    resp->add_supported_languages(plugin.language);
  }

  for(const SourceGroupType type : registry->availableSourceGroupTypes()) {
    resp->add_supported_source_group_types(type);
  }

  // Custom Command is supported with no plugin installed at all: the user supplies the command line,
  // so the engine only has to run it. This is why can_create_project stays true on a bare install --
  // the client goes read-only when the engine is *unreachable*, not when it is empty.
  resp->add_supported_source_group_types(SOURCE_GROUP_CUSTOM_COMMAND);
  resp->add_supported_languages(LANGUAGE_CUSTOM);

  resp->set_can_create_project(resp->supported_source_group_types_size() > 0);
  resp->set_engine_version(Version::getApplicationVersion().toString());
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetCompilationDatabaseInfo(grpc::ServerContext* /*ctx*/,
                                                           const sourcetrail::CompilationDatabaseInfoRequest* req,
                                                           sourcetrail::CompilationDatabaseInfoResponse* resp) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  const FilePath cdbPath(utility::decodeFromUtf8(req->cdb_path()));
  if(cdbPath.empty() || !cdbPath.exists()) {
    resp->set_error("The provided Compilation Database path does not exist.");
    return grpc::Status::OK;
  }

  std::string error;
  const std::shared_ptr<clang::tooling::JSONCompilationDatabase> cdb = utility::loadCDB(cdbPath, &error);
  if(!cdb) {
    resp->set_error(error.empty() ? "Unable to open and read the provided compilation database file." : error);
    return grpc::Status::OK;
  }

  resp->set_valid(true);
  for(const FilePath& path : utility::getSourceFilesFromCDB(cdb, cdbPath)) {
    resp->add_source_files(utility::encodeToUtf8(path.wstr()));
  }
  for(const FilePath& path : utility::CompilationDatabase(cdbPath).getAllHeaderPaths()) {
    resp->add_header_paths(utility::encodeToUtf8(path.wstr()));
  }
  resp->set_contains_include_pch_flags(utility::containsIncludePchFlags(cdb));
#else
  (void)req;
  resp->set_error("This engine was built without the C/C++ language package.");
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  return grpc::Status::OK;
}

void EngineServiceImpl::setShutdownHandler(std::function<void()> handler) {
  mShutdownHandler = std::move(handler);
}

grpc::Status EngineServiceImpl::Shutdown(grpc::ServerContext* /*ctx*/,
                                         const sourcetrail::EmptyRequest* /*req*/,
                                         sourcetrail::EmptyResponse* /*resp*/) {
  if(mShutdownHandler) {
    mShutdownHandler();
  }
  return grpc::Status::OK;
}

// ---- Bookmark mutations ----------------------------------------------------

grpc::Status EngineServiceImpl::AddNodeBookmark(grpc::ServerContext* /*ctx*/,
                                                const sourcetrail::AddNodeBookmarkRequest* req,
                                                sourcetrail::BookmarkIdResponse* resp) {
  sourcetrail::ProtoNodeBookmark msg;
  *msg.mutable_base() = req->base();
  *msg.mutable_node_ids() = req->node_ids();
  resp->set_id(mStorageAccess->addNodeBookmark(proto::convert::fromProto(msg)));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::AddEdgeBookmark(grpc::ServerContext* /*ctx*/,
                                                const sourcetrail::AddEdgeBookmarkRequest* req,
                                                sourcetrail::BookmarkIdResponse* resp) {
  sourcetrail::ProtoEdgeBookmark msg;
  *msg.mutable_base() = req->base();
  *msg.mutable_edge_ids() = req->edge_ids();
  msg.set_active_node_id(req->active_node_id());
  resp->set_id(mStorageAccess->addEdgeBookmark(proto::convert::fromProto(msg)));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::AddBookmarkCategory(grpc::ServerContext* /*ctx*/,
                                                    const sourcetrail::AddBookmarkCategoryRequest* req,
                                                    sourcetrail::BookmarkIdResponse* resp) {
  const Id id = mStorageAccess->addBookmarkCategory(utility::decodeFromUtf8(req->name()));
  resp->set_id(id);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::UpdateBookmark(grpc::ServerContext* /*ctx*/,
                                               const sourcetrail::UpdateBookmarkRequest* req,
                                               sourcetrail::EmptyResponse* /*resp*/) {
  mStorageAccess->updateBookmark(static_cast<Id>(req->bookmark_id()),
                                 utility::decodeFromUtf8(req->name()),
                                 utility::decodeFromUtf8(req->comment()),
                                 utility::decodeFromUtf8(req->category_name()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::RemoveBookmark(grpc::ServerContext* /*ctx*/,
                                               const sourcetrail::RemoveBookmarkRequest* req,
                                               sourcetrail::EmptyResponse* /*resp*/) {
  mStorageAccess->removeBookmark(static_cast<Id>(req->bookmark_id()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::RemoveBookmarkCategory(grpc::ServerContext* /*ctx*/,
                                                       const sourcetrail::RemoveBookmarkCategoryRequest* req,
                                                       sourcetrail::EmptyResponse* /*resp*/) {
  mStorageAccess->removeBookmarkCategory(static_cast<Id>(req->category_id()));
  return grpc::Status::OK;
}

// ---- Storage queries — Phase 4 implementations ----------------------------

grpc::Status EngineServiceImpl::GetNodeIdForFileNode(grpc::ServerContext* /*ctx*/,
                                                     const sourcetrail::FilePathRequest* req,
                                                     sourcetrail::IdResponse* resp) {
  resp->set_id(mStorageAccess->getNodeIdForFileNode(FilePath(utility::decodeFromUtf8(req->file_path()))));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNodeIdForNameHierarchy(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::NameHierarchyRequest* req,
                                                          sourcetrail::IdResponse* resp) {
  resp->set_id(mStorageAccess->getNodeIdForNameHierarchy(NameHierarchy::deserialize(utility::decodeFromUtf8(req->serialized()))));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNodeIdsForNameHierarchies(grpc::ServerContext* /*ctx*/,
                                                             const sourcetrail::NameHierarchiesRequest* req,
                                                             sourcetrail::IdsResponse* resp) {
  std::vector<NameHierarchy> hierarchies;
  hierarchies.reserve(static_cast<size_t>(req->serialized_size()));
  for(const auto& s : req->serialized()) {
    hierarchies.push_back(NameHierarchy::deserialize(utility::decodeFromUtf8(s)));
  }
  for(const Id id : mStorageAccess->getNodeIdsForNameHierarchies(hierarchies)) {
    resp->add_ids(id);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNameHierarchyForNodeId(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::IdRequest* req,
                                                          sourcetrail::NameHierarchyResponse* resp) {
  const NameHierarchy nh = mStorageAccess->getNameHierarchyForNodeId(static_cast<Id>(req->id()));
  resp->set_serialized(utility::encodeToUtf8(NameHierarchy::serialize(nh)));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNameHierarchiesForNodeIds(grpc::ServerContext* /*ctx*/,
                                                             const sourcetrail::IdsRequest* req,
                                                             sourcetrail::NameHierarchiesResponse* resp) {
  std::vector<Id> ids(req->ids().begin(), req->ids().end());
  for(const NameHierarchy& nh : mStorageAccess->getNameHierarchiesForNodeIds(ids)) {
    resp->add_serialized(utility::encodeToUtf8(NameHierarchy::serialize(nh)));
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNodeIdToParentFileMap(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::IdsRequest* req,
                                                         sourcetrail::NodeIdToParentFileMapResponse* resp) {
  std::vector<Id> ids(req->ids().begin(), req->ids().end());
  for(const auto& [nodeId, pair] : mStorageAccess->getNodeIdToParentFileMap(ids)) {
    auto* entry = resp->add_entries();
    entry->set_node_id(nodeId);
    entry->set_parent_file_id(pair.first);
    entry->set_name_hierarchy_serialized(utility::encodeToUtf8(NameHierarchy::serialize(pair.second)));
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNodeTypeForNodeWithId(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::IdRequest* req,
                                                         sourcetrail::NodeKindResponse* resp) {
  const NodeType nt = mStorageAccess->getNodeTypeForNodeWithId(static_cast<Id>(req->id()));
  resp->set_kind(nodeKindToInt(nt.getKind()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetEdgeById(grpc::ServerContext* /*ctx*/,
                                            const sourcetrail::IdRequest* req,
                                            sourcetrail::StorageEdgeResponse* resp) {
  const StorageEdge edge = mStorageAccess->getEdgeById(static_cast<Id>(req->id()));
  resp->mutable_edge()->set_id(edge.id);
  resp->mutable_edge()->set_type(edge.type);
  resp->mutable_edge()->set_source_node_id(edge.sourceNodeId);
  resp->mutable_edge()->set_target_node_id(edge.targetNodeId);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetFullTextSearchLocations(grpc::ServerContext* /*ctx*/,
                                                           const sourcetrail::FullTextSearchRequest* req,
                                                           sourcetrail::SourceLocationCollectionResponse* resp) {
  const auto collection = mStorageAccess->getFullTextSearchLocations(
      utility::decodeFromUtf8(req->search_term()), req->case_sensitive());
  if(collection) {
    *resp = proto::convert::toProto(*collection);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAutocompletionMatches(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::AutocompletionRequest* req,
                                                         sourcetrail::SearchMatchesResponse* resp) {
  const auto matches = mStorageAccess->getAutocompletionMatches(
      utility::decodeFromUtf8(req->query()),
      proto::convert::nodeTypeSetFromMask(req->accepted_node_types_mask()),
      req->accept_commands());
  for(const SearchMatch& match : matches) {
    *resp->add_matches() = proto::convert::toProto(match);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSearchMatchesForTokenIds(grpc::ServerContext* /*ctx*/,
                                                            const sourcetrail::IdsRequest* req,
                                                            sourcetrail::SearchMatchesResponse* resp) {
  const std::vector<Id> tokenIds(req->ids().begin(), req->ids().end());
  for(const SearchMatch& match : mStorageAccess->getSearchMatchesForTokenIds(tokenIds)) {
    *resp->add_matches() = proto::convert::toProto(match);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetGraphForAll(grpc::ServerContext* /*ctx*/,
                                               const sourcetrail::EmptyRequest* /*req*/,
                                               sourcetrail::GraphResponse* resp) {
  if(const auto graph = mStorageAccess->getGraphForAll(); graph) {
    *resp->mutable_graph() = proto::convert::toProto(*graph);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetGraphForNodeTypes(grpc::ServerContext* /*ctx*/,
                                                     const sourcetrail::NodeKindMaskRequest* req,
                                                     sourcetrail::GraphResponse* resp) {
  const auto graph = mStorageAccess->getGraphForNodeTypes(proto::convert::nodeTypeSetFromMask(req->mask()));
  if(graph) {
    *resp->mutable_graph() = proto::convert::toProto(*graph);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetGraphForActiveTokenIds(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::ActiveTokensRequest* req,
                                                          sourcetrail::ActiveGraphResponse* resp) {
  const std::vector<Id> tokenIds(req->token_ids().begin(), req->token_ids().end());
  const std::vector<Id> expandedNodeIds(req->expanded_node_ids().begin(), req->expanded_node_ids().end());

  bool isActiveNamespace = false;
  const auto graph = mStorageAccess->getGraphForActiveTokenIds(tokenIds, expandedNodeIds, &isActiveNamespace);
  if(graph) {
    *resp->mutable_graph() = proto::convert::toProto(*graph);
  }
  resp->set_is_active_namespace(isActiveNamespace);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetGraphForChildrenOfNodeId(grpc::ServerContext* /*ctx*/,
                                                            const sourcetrail::IdRequest* req,
                                                            sourcetrail::GraphResponse* resp) {
  if(const auto graph = mStorageAccess->getGraphForChildrenOfNodeId(static_cast<Id>(req->id())); graph) {
    *resp->mutable_graph() = proto::convert::toProto(*graph);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetGraphForTrail(grpc::ServerContext* /*ctx*/,
                                                 const sourcetrail::TrailRequest* req,
                                                 sourcetrail::GraphResponse* resp) {
  const auto graph = mStorageAccess->getGraphForTrail(req->origin_id(),
                                                      req->target_id(),
                                                      req->node_types_mask(),
                                                      req->edge_types_mask(),
                                                      req->node_non_indexed(),
                                                      req->depth(),
                                                      req->directed());
  if(graph) {
    *resp->mutable_graph() = proto::convert::toProto(*graph);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAvailableNodeTypes(grpc::ServerContext* /*ctx*/,
                                                      const sourcetrail::EmptyRequest* /*req*/,
                                                      sourcetrail::NodeKindMaskResponse* resp) {
  resp->set_mask(static_cast<int32_t>(mStorageAccess->getAvailableNodeTypes()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAvailableEdgeTypes(grpc::ServerContext* /*ctx*/,
                                                      const sourcetrail::EmptyRequest* /*req*/,
                                                      sourcetrail::EdgeTypeMaskResponse* resp) {
  resp->set_mask(static_cast<int32_t>(mStorageAccess->getAvailableEdgeTypes()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetActiveTokenIdsForId(grpc::ServerContext* /*ctx*/,
                                                       const sourcetrail::ActiveTokenIdRequest* req,
                                                       sourcetrail::ActiveTokenIdsResponse* resp) {
  Id declarationId = 0;
  const std::vector<Id> ids = mStorageAccess->getActiveTokenIdsForId(static_cast<Id>(req->token_id()), &declarationId);
  for(const Id id : ids) {
    resp->add_ids(id);
  }
  resp->set_declaration_id(declarationId);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetNodeIdsForLocationIds(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::IdsRequest* req,
                                                         sourcetrail::IdsResponse* resp) {
  std::vector<Id> locationIds(req->ids().begin(), req->ids().end());
  for(const Id id : mStorageAccess->getNodeIdsForLocationIds(locationIds)) {
    resp->add_ids(id);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSourceLocationsForTokenIds(grpc::ServerContext* /*ctx*/,
                                                              const sourcetrail::IdsRequest* req,
                                                              sourcetrail::SourceLocationCollectionResponse* resp) {
  const std::vector<Id> tokenIds(req->ids().begin(), req->ids().end());
  if(const auto collection = mStorageAccess->getSourceLocationsForTokenIds(tokenIds); collection) {
    *resp = proto::convert::toProto(*collection);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSourceLocationsForLocationIds(grpc::ServerContext* /*ctx*/,
                                                                 const sourcetrail::IdsRequest* req,
                                                                 sourcetrail::SourceLocationCollectionResponse* resp) {
  const std::vector<Id> locationIds(req->ids().begin(), req->ids().end());
  if(const auto collection = mStorageAccess->getSourceLocationsForLocationIds(locationIds); collection) {
    *resp = proto::convert::toProto(*collection);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSourceLocationsForFile(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::FilePathRequest* req,
                                                          sourcetrail::SourceLocationFileResponse* resp) {
  const auto file = mStorageAccess->getSourceLocationsForFile(FilePath(utility::decodeFromUtf8(req->file_path())));
  if(file) {
    *resp->mutable_file() = proto::convert::toProto(*file);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSourceLocationsForLinesInFile(grpc::ServerContext* /*ctx*/,
                                                                 const sourcetrail::LinesInFileRequest* req,
                                                                 sourcetrail::SourceLocationFileResponse* resp) {
  const auto file = mStorageAccess->getSourceLocationsForLinesInFile(
      FilePath(utility::decodeFromUtf8(req->file_path())), req->start_line(), req->end_line());
  if(file) {
    *resp->mutable_file() = proto::convert::toProto(*file);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetSourceLocationsOfTypeInFile(grpc::ServerContext* /*ctx*/,
                                                               const sourcetrail::LocationTypeInFileRequest* req,
                                                               sourcetrail::SourceLocationFileResponse* resp) {
  const auto file = mStorageAccess->getSourceLocationsOfTypeInFile(
      FilePath(utility::decodeFromUtf8(req->file_path())), static_cast<LocationType>(req->location_type()));
  if(file) {
    *resp->mutable_file() = proto::convert::toProto(*file);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetFileContent(grpc::ServerContext* /*ctx*/,
                                               const sourcetrail::FileContentRequest* req,
                                               sourcetrail::FileContentResponse* resp) {
  auto textAccess = mStorageAccess->getFileContent(FilePath(utility::decodeFromUtf8(req->file_path())), req->shows_errors());
  if(textAccess) {
    resp->set_content(textAccess->getText());
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetFileInfoForFileId(grpc::ServerContext* /*ctx*/,
                                                     const sourcetrail::IdRequest* req,
                                                     sourcetrail::FileInfoResponse* resp) {
  *resp->mutable_file_info() = proto::convert::toProto(mStorageAccess->getFileInfoForFileId(req->id()));
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetFileInfoForFilePath(grpc::ServerContext* /*ctx*/,
                                                       const sourcetrail::FilePathRequest* req,
                                                       sourcetrail::FileInfoResponse* resp) {
  const FileInfo info = mStorageAccess->getFileInfoForFilePath(FilePath(utility::decodeFromUtf8(req->file_path())));
  *resp->mutable_file_info() = proto::convert::toProto(info);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetFileInfosForFilePaths(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::FilePathsRequest* req,
                                                         sourcetrail::FileInfosResponse* resp) {
  std::vector<FilePath> filePaths;
  filePaths.reserve(static_cast<size_t>(req->file_paths_size()));
  for(const auto& path : req->file_paths()) {
    filePaths.emplace_back(utility::decodeFromUtf8(path));
  }
  for(const FileInfo& info : mStorageAccess->getFileInfosForFilePaths(filePaths)) {
    *resp->add_file_infos() = proto::convert::toProto(info);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetStorageStats(grpc::ServerContext* /*ctx*/,
                                                const sourcetrail::EmptyRequest* /*req*/,
                                                sourcetrail::StorageStatsResponse* resp) {
  const StorageStats stats = mStorageAccess->getStorageStats();
  auto* protoStats = resp->mutable_stats();
  protoStats->set_node_count(stats.nodeCount);
  protoStats->set_edge_count(stats.edgeCount);
  protoStats->set_file_count(stats.fileCount);
  protoStats->set_completed_file_count(stats.completedFileCount);
  protoStats->set_file_loc_count(stats.fileLOCCount);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetErrorCount(grpc::ServerContext* /*ctx*/,
                                              const sourcetrail::EmptyRequest* /*req*/,
                                              sourcetrail::ErrorCountResponse* resp) {
  const ErrorCountInfo info = mStorageAccess->getErrorCount();
  resp->set_total(info.total);
  resp->set_fatal(info.fatal);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetErrorsLimited(grpc::ServerContext* /*ctx*/,
                                                 const sourcetrail::ErrorFilterRequest* req,
                                                 sourcetrail::ErrorInfosResponse* resp) {
  const ErrorFilter filter = proto::convert::fromProto(req->filter());
  for(const ErrorInfo& error : mStorageAccess->getErrorsLimited(filter)) {
    *resp->add_errors() = proto::convert::toProto(error);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetErrorsForFileLimited(grpc::ServerContext* /*ctx*/,
                                                        const sourcetrail::ErrorFilterFileRequest* req,
                                                        sourcetrail::ErrorInfosResponse* resp) {
  const ErrorFilter filter = proto::convert::fromProto(req->filter());
  const FilePath filePath(utility::decodeFromUtf8(req->file_path()));
  for(const ErrorInfo& error : mStorageAccess->getErrorsForFileLimited(filter, filePath)) {
    *resp->add_errors() = proto::convert::toProto(error);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetErrorSourceLocations(grpc::ServerContext* /*ctx*/,
                                                        const sourcetrail::ErrorInfosRequest* req,
                                                        sourcetrail::SourceLocationCollectionResponse* resp) {
  std::vector<ErrorInfo> errors;
  errors.reserve(static_cast<size_t>(req->errors_size()));
  for(const auto& errorMsg : req->errors()) {
    errors.push_back(proto::convert::fromProto(errorMsg));
  }
  if(const auto collection = mStorageAccess->getErrorSourceLocations(errors); collection) {
    *resp = proto::convert::toProto(*collection);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAllNodeBookmarks(grpc::ServerContext* /*ctx*/,
                                                    const sourcetrail::EmptyRequest* /*req*/,
                                                    sourcetrail::NodeBookmarksResponse* resp) {
  for(const NodeBookmark& bookmark : mStorageAccess->getAllNodeBookmarks()) {
    *resp->add_bookmarks() = proto::convert::toProto(bookmark);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAllEdgeBookmarks(grpc::ServerContext* /*ctx*/,
                                                    const sourcetrail::EmptyRequest* /*req*/,
                                                    sourcetrail::EdgeBookmarksResponse* resp) {
  for(const EdgeBookmark& bookmark : mStorageAccess->getAllEdgeBookmarks()) {
    *resp->add_bookmarks() = proto::convert::toProto(bookmark);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetAllBookmarkCategories(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::EmptyRequest* /*req*/,
                                                         sourcetrail::BookmarkCategoriesResponse* resp) {
  for(const BookmarkCategory& category : mStorageAccess->getAllBookmarkCategories()) {
    *resp->add_categories() = proto::convert::toProto(category);
  }
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetTooltipInfoForTokenIds(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::TooltipTokenIdsRequest* req,
                                                          sourcetrail::TooltipInfoResponse* resp) {
  const std::vector<Id> tokenIds(req->token_ids().begin(), req->token_ids().end());
  const TooltipInfo info = mStorageAccess->getTooltipInfoForTokenIds(tokenIds, static_cast<TooltipOrigin>(req->origin()));
  *resp->mutable_info() = proto::convert::toProto(info);
  return grpc::Status::OK;
}

grpc::Status EngineServiceImpl::GetTooltipInfoForSourceLocationIdsAndLocalSymbolIds(grpc::ServerContext* /*ctx*/,
                                                                                    const sourcetrail::TooltipLocationRequest* req,
                                                                                    sourcetrail::TooltipInfoResponse* resp) {
  const std::vector<Id> locationIds(req->location_ids().begin(), req->location_ids().end());
  const std::vector<Id> localSymbolIds(req->local_symbol_ids().begin(), req->local_symbol_ids().end());
  const TooltipInfo info = mStorageAccess->getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(locationIds, localSymbolIds);
  *resp->mutable_info() = proto::convert::toProto(info);
  return grpc::Status::OK;
}

// ---- Event stream ----------------------------------------------------------

grpc::Status EngineServiceImpl::WatchEvents(grpc::ServerContext* ctx,
                                            const sourcetrail::WatchEventsRequest* /*req*/,
                                            grpc::ServerWriter<sourcetrail::EngineEvent>* writer) {
  {
    const std::lock_guard<std::mutex> lock(mEventListenersMutex);
    mEventListeners.push_back(writer);
  }

  while(!ctx->IsCancelled()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  {
    const std::lock_guard<std::mutex> lock(mEventListenersMutex);
    mEventListeners.erase(std::remove(mEventListeners.begin(), mEventListeners.end(), writer), mEventListeners.end());
  }

  return grpc::Status::OK;
}

void EngineServiceImpl::broadcastEvent(const sourcetrail::EngineEvent& event) {
  const std::lock_guard<std::mutex> lock(mEventListenersMutex);
  for(auto* writer : mEventListeners) {
    writer->Write(event);
  }
}
