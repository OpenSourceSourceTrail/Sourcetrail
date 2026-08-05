#include "EngineServiceImpl.h"

#include <grpcpp/grpcpp.h>

#include "FilePath.h"
#include "NameHierarchy.h"
#include "NodeKind.h"
#include "NodeType.h"
#include "StorageAccess.h"
#include "TextAccess.h"
#include "logging.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageRefresh.h"
#include "utilityString.h"

EngineServiceImpl::EngineServiceImpl(StorageAccess* storageAccess) : mStorageAccess(storageAccess) {}

// ---- Lifecycle -------------------------------------------------------------

grpc::Status EngineServiceImpl::LoadProject(grpc::ServerContext* /*ctx*/,
                                             const sourcetrail::LoadProjectRequest* req,
                                             sourcetrail::LoadProjectResponse* /*resp*/) {
  MessageLoadProject{FilePath(utility::decodeFromUtf8(req->project_file_path())), false}.dispatch();
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
                                                 const sourcetrail::AddNodeBookmarkRequest* /*req*/,
                                                 sourcetrail::BookmarkIdResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::AddEdgeBookmark(grpc::ServerContext* /*ctx*/,
                                                 const sourcetrail::AddEdgeBookmarkRequest* /*req*/,
                                                 sourcetrail::BookmarkIdResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
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

// Complex query types (Graph, SourceLocationCollection) — Phase 4 TODO
grpc::Status EngineServiceImpl::GetFullTextSearchLocations(grpc::ServerContext* /*ctx*/,
                                                            const sourcetrail::FullTextSearchRequest* /*req*/,
                                                            sourcetrail::SourceLocationCollectionResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetAutocompletionMatches(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::AutocompletionRequest* /*req*/,
                                                          sourcetrail::SearchMatchesResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetSearchMatchesForTokenIds(grpc::ServerContext* /*ctx*/,
                                                             const sourcetrail::IdsRequest* /*req*/,
                                                             sourcetrail::SearchMatchesResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetGraphForAll(grpc::ServerContext* /*ctx*/,
                                                const sourcetrail::EmptyRequest* /*req*/,
                                                sourcetrail::GraphResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetGraphForNodeTypes(grpc::ServerContext* /*ctx*/,
                                                      const sourcetrail::NodeKindMaskRequest* /*req*/,
                                                      sourcetrail::GraphResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetGraphForActiveTokenIds(grpc::ServerContext* /*ctx*/,
                                                           const sourcetrail::ActiveTokensRequest* /*req*/,
                                                           sourcetrail::ActiveGraphResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetGraphForChildrenOfNodeId(grpc::ServerContext* /*ctx*/,
                                                             const sourcetrail::IdRequest* /*req*/,
                                                             sourcetrail::GraphResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetGraphForTrail(grpc::ServerContext* /*ctx*/,
                                                  const sourcetrail::TrailRequest* /*req*/,
                                                  sourcetrail::GraphResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
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
                                                               const sourcetrail::IdsRequest* /*req*/,
                                                               sourcetrail::SourceLocationCollectionResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetSourceLocationsForLocationIds(grpc::ServerContext* /*ctx*/,
                                                                  const sourcetrail::IdsRequest* /*req*/,
                                                                  sourcetrail::SourceLocationCollectionResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetSourceLocationsForFile(grpc::ServerContext* /*ctx*/,
                                                           const sourcetrail::FilePathRequest* /*req*/,
                                                           sourcetrail::SourceLocationFileResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetSourceLocationsForLinesInFile(grpc::ServerContext* /*ctx*/,
                                                                   const sourcetrail::LinesInFileRequest* /*req*/,
                                                                   sourcetrail::SourceLocationFileResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetSourceLocationsOfTypeInFile(grpc::ServerContext* /*ctx*/,
                                                                const sourcetrail::LocationTypeInFileRequest* /*req*/,
                                                                sourcetrail::SourceLocationFileResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
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
                                                      const sourcetrail::IdRequest* /*req*/,
                                                      sourcetrail::FileInfoResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetFileInfoForFilePath(grpc::ServerContext* /*ctx*/,
                                                        const sourcetrail::FilePathRequest* /*req*/,
                                                        sourcetrail::FileInfoResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetFileInfosForFilePaths(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::FilePathsRequest* /*req*/,
                                                          sourcetrail::FileInfosResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
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
                                                   const sourcetrail::ErrorFilterRequest* /*req*/,
                                                   sourcetrail::ErrorInfosResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetErrorsForFileLimited(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::ErrorFilterFileRequest* /*req*/,
                                                          sourcetrail::ErrorInfosResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetErrorSourceLocations(grpc::ServerContext* /*ctx*/,
                                                         const sourcetrail::ErrorInfosRequest* /*req*/,
                                                         sourcetrail::SourceLocationCollectionResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetAllNodeBookmarks(grpc::ServerContext* /*ctx*/,
                                                     const sourcetrail::EmptyRequest* /*req*/,
                                                     sourcetrail::NodeBookmarksResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetAllEdgeBookmarks(grpc::ServerContext* /*ctx*/,
                                                     const sourcetrail::EmptyRequest* /*req*/,
                                                     sourcetrail::EdgeBookmarksResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetAllBookmarkCategories(grpc::ServerContext* /*ctx*/,
                                                          const sourcetrail::EmptyRequest* /*req*/,
                                                          sourcetrail::BookmarkCategoriesResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetTooltipInfoForTokenIds(grpc::ServerContext* /*ctx*/,
                                                           const sourcetrail::TooltipTokenIdsRequest* /*req*/,
                                                           sourcetrail::TooltipInfoResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
}

grpc::Status EngineServiceImpl::GetTooltipInfoForSourceLocationIdsAndLocalSymbolIds(
    grpc::ServerContext* /*ctx*/,
    const sourcetrail::TooltipLocationRequest* /*req*/,
    sourcetrail::TooltipInfoResponse* /*resp*/) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Phase 4");
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
