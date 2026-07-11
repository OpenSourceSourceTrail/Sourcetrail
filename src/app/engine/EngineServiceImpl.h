#pragma once
#include <memory>

#include <grpcpp/grpcpp.h>

#include "engine.grpc.pb.h"

class StorageAccess;
class Application;

class EngineServiceImpl final : public sourcetrail::EngineService::Service {
public:
  explicit EngineServiceImpl(StorageAccess* storageAccess);
  ~EngineServiceImpl() override = default;

  // Lifecycle
  grpc::Status LoadProject(grpc::ServerContext* ctx,
                           const sourcetrail::LoadProjectRequest* req,
                           sourcetrail::LoadProjectResponse* resp) override;

  grpc::Status Refresh(grpc::ServerContext* ctx,
                       const sourcetrail::RefreshRequest* req,
                       sourcetrail::RefreshResponse* resp) override;

  grpc::Status IndexingInterrupted(grpc::ServerContext* ctx,
                                   const sourcetrail::IndexingInterruptedRequest* req,
                                   sourcetrail::IndexingInterruptedResponse* resp) override;

  // Bookmark mutations
  grpc::Status AddNodeBookmark(grpc::ServerContext* ctx,
                               const sourcetrail::AddNodeBookmarkRequest* req,
                               sourcetrail::BookmarkIdResponse* resp) override;

  grpc::Status AddEdgeBookmark(grpc::ServerContext* ctx,
                               const sourcetrail::AddEdgeBookmarkRequest* req,
                               sourcetrail::BookmarkIdResponse* resp) override;

  grpc::Status AddBookmarkCategory(grpc::ServerContext* ctx,
                                   const sourcetrail::AddBookmarkCategoryRequest* req,
                                   sourcetrail::BookmarkIdResponse* resp) override;

  grpc::Status UpdateBookmark(grpc::ServerContext* ctx,
                              const sourcetrail::UpdateBookmarkRequest* req,
                              sourcetrail::EmptyResponse* resp) override;

  grpc::Status RemoveBookmark(grpc::ServerContext* ctx,
                              const sourcetrail::RemoveBookmarkRequest* req,
                              sourcetrail::EmptyResponse* resp) override;

  grpc::Status RemoveBookmarkCategory(grpc::ServerContext* ctx,
                                      const sourcetrail::RemoveBookmarkCategoryRequest* req,
                                      sourcetrail::EmptyResponse* resp) override;

  // Storage queries — Phase 4 fills these in; Phase 3 returns UNIMPLEMENTED
  grpc::Status GetNodeIdForFileNode(grpc::ServerContext* ctx,
                                    const sourcetrail::FilePathRequest* req,
                                    sourcetrail::IdResponse* resp) override;

  grpc::Status GetNodeIdForNameHierarchy(grpc::ServerContext* ctx,
                                         const sourcetrail::NameHierarchyRequest* req,
                                         sourcetrail::IdResponse* resp) override;

  grpc::Status GetNodeIdsForNameHierarchies(grpc::ServerContext* ctx,
                                            const sourcetrail::NameHierarchiesRequest* req,
                                            sourcetrail::IdsResponse* resp) override;

  grpc::Status GetNameHierarchyForNodeId(grpc::ServerContext* ctx,
                                          const sourcetrail::IdRequest* req,
                                          sourcetrail::NameHierarchyResponse* resp) override;

  grpc::Status GetNameHierarchiesForNodeIds(grpc::ServerContext* ctx,
                                             const sourcetrail::IdsRequest* req,
                                             sourcetrail::NameHierarchiesResponse* resp) override;

  grpc::Status GetNodeIdToParentFileMap(grpc::ServerContext* ctx,
                                         const sourcetrail::IdsRequest* req,
                                         sourcetrail::NodeIdToParentFileMapResponse* resp) override;

  grpc::Status GetNodeTypeForNodeWithId(grpc::ServerContext* ctx,
                                         const sourcetrail::IdRequest* req,
                                         sourcetrail::NodeKindResponse* resp) override;

  grpc::Status GetEdgeById(grpc::ServerContext* ctx,
                            const sourcetrail::IdRequest* req,
                            sourcetrail::StorageEdgeResponse* resp) override;

  grpc::Status GetFullTextSearchLocations(grpc::ServerContext* ctx,
                                           const sourcetrail::FullTextSearchRequest* req,
                                           sourcetrail::SourceLocationCollectionResponse* resp) override;

  grpc::Status GetAutocompletionMatches(grpc::ServerContext* ctx,
                                         const sourcetrail::AutocompletionRequest* req,
                                         sourcetrail::SearchMatchesResponse* resp) override;

  grpc::Status GetSearchMatchesForTokenIds(grpc::ServerContext* ctx,
                                            const sourcetrail::IdsRequest* req,
                                            sourcetrail::SearchMatchesResponse* resp) override;

  grpc::Status GetGraphForAll(grpc::ServerContext* ctx,
                               const sourcetrail::EmptyRequest* req,
                               sourcetrail::GraphResponse* resp) override;

  grpc::Status GetGraphForNodeTypes(grpc::ServerContext* ctx,
                                     const sourcetrail::NodeKindMaskRequest* req,
                                     sourcetrail::GraphResponse* resp) override;

  grpc::Status GetGraphForActiveTokenIds(grpc::ServerContext* ctx,
                                          const sourcetrail::ActiveTokensRequest* req,
                                          sourcetrail::ActiveGraphResponse* resp) override;

  grpc::Status GetGraphForChildrenOfNodeId(grpc::ServerContext* ctx,
                                            const sourcetrail::IdRequest* req,
                                            sourcetrail::GraphResponse* resp) override;

  grpc::Status GetGraphForTrail(grpc::ServerContext* ctx,
                                 const sourcetrail::TrailRequest* req,
                                 sourcetrail::GraphResponse* resp) override;

  grpc::Status GetAvailableNodeTypes(grpc::ServerContext* ctx,
                                      const sourcetrail::EmptyRequest* req,
                                      sourcetrail::NodeKindMaskResponse* resp) override;

  grpc::Status GetAvailableEdgeTypes(grpc::ServerContext* ctx,
                                      const sourcetrail::EmptyRequest* req,
                                      sourcetrail::EdgeTypeMaskResponse* resp) override;

  grpc::Status GetActiveTokenIdsForId(grpc::ServerContext* ctx,
                                       const sourcetrail::ActiveTokenIdRequest* req,
                                       sourcetrail::ActiveTokenIdsResponse* resp) override;

  grpc::Status GetNodeIdsForLocationIds(grpc::ServerContext* ctx,
                                         const sourcetrail::IdsRequest* req,
                                         sourcetrail::IdsResponse* resp) override;

  grpc::Status GetSourceLocationsForTokenIds(grpc::ServerContext* ctx,
                                              const sourcetrail::IdsRequest* req,
                                              sourcetrail::SourceLocationCollectionResponse* resp) override;

  grpc::Status GetSourceLocationsForLocationIds(grpc::ServerContext* ctx,
                                                 const sourcetrail::IdsRequest* req,
                                                 sourcetrail::SourceLocationCollectionResponse* resp) override;

  grpc::Status GetSourceLocationsForFile(grpc::ServerContext* ctx,
                                          const sourcetrail::FilePathRequest* req,
                                          sourcetrail::SourceLocationFileResponse* resp) override;

  grpc::Status GetSourceLocationsForLinesInFile(grpc::ServerContext* ctx,
                                                 const sourcetrail::LinesInFileRequest* req,
                                                 sourcetrail::SourceLocationFileResponse* resp) override;

  grpc::Status GetSourceLocationsOfTypeInFile(grpc::ServerContext* ctx,
                                               const sourcetrail::LocationTypeInFileRequest* req,
                                               sourcetrail::SourceLocationFileResponse* resp) override;

  grpc::Status GetFileContent(grpc::ServerContext* ctx,
                               const sourcetrail::FileContentRequest* req,
                               sourcetrail::FileContentResponse* resp) override;

  grpc::Status GetFileInfoForFileId(grpc::ServerContext* ctx,
                                     const sourcetrail::IdRequest* req,
                                     sourcetrail::FileInfoResponse* resp) override;

  grpc::Status GetFileInfoForFilePath(grpc::ServerContext* ctx,
                                       const sourcetrail::FilePathRequest* req,
                                       sourcetrail::FileInfoResponse* resp) override;

  grpc::Status GetFileInfosForFilePaths(grpc::ServerContext* ctx,
                                         const sourcetrail::FilePathsRequest* req,
                                         sourcetrail::FileInfosResponse* resp) override;

  grpc::Status GetStorageStats(grpc::ServerContext* ctx,
                                const sourcetrail::EmptyRequest* req,
                                sourcetrail::StorageStatsResponse* resp) override;

  grpc::Status GetErrorCount(grpc::ServerContext* ctx,
                              const sourcetrail::EmptyRequest* req,
                              sourcetrail::ErrorCountResponse* resp) override;

  grpc::Status GetErrorsLimited(grpc::ServerContext* ctx,
                                 const sourcetrail::ErrorFilterRequest* req,
                                 sourcetrail::ErrorInfosResponse* resp) override;

  grpc::Status GetErrorsForFileLimited(grpc::ServerContext* ctx,
                                        const sourcetrail::ErrorFilterFileRequest* req,
                                        sourcetrail::ErrorInfosResponse* resp) override;

  grpc::Status GetErrorSourceLocations(grpc::ServerContext* ctx,
                                        const sourcetrail::ErrorInfosRequest* req,
                                        sourcetrail::SourceLocationCollectionResponse* resp) override;

  grpc::Status GetAllNodeBookmarks(grpc::ServerContext* ctx,
                                    const sourcetrail::EmptyRequest* req,
                                    sourcetrail::NodeBookmarksResponse* resp) override;

  grpc::Status GetAllEdgeBookmarks(grpc::ServerContext* ctx,
                                    const sourcetrail::EmptyRequest* req,
                                    sourcetrail::EdgeBookmarksResponse* resp) override;

  grpc::Status GetAllBookmarkCategories(grpc::ServerContext* ctx,
                                         const sourcetrail::EmptyRequest* req,
                                         sourcetrail::BookmarkCategoriesResponse* resp) override;

  grpc::Status GetTooltipInfoForTokenIds(grpc::ServerContext* ctx,
                                          const sourcetrail::TooltipTokenIdsRequest* req,
                                          sourcetrail::TooltipInfoResponse* resp) override;

  grpc::Status GetTooltipInfoForSourceLocationIdsAndLocalSymbolIds(grpc::ServerContext* ctx,
                                                                    const sourcetrail::TooltipLocationRequest* req,
                                                                    sourcetrail::TooltipInfoResponse* resp) override;

  // Event stream
  grpc::Status WatchEvents(grpc::ServerContext* ctx,
                            const sourcetrail::WatchEventsRequest* req,
                            grpc::ServerWriter<sourcetrail::EngineEvent>* writer) override;

  // Called by the engine when an event occurs (indexing progress, status, etc.)
  void broadcastEvent(const sourcetrail::EngineEvent& event);

private:
  StorageAccess* mStorageAccess;

  std::mutex mEventListenersMutex;
  std::vector<grpc::ServerWriter<sourcetrail::EngineEvent>*> mEventListeners;
};
