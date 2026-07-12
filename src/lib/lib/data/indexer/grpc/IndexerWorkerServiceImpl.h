#pragma once
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "indexer_worker.grpc.pb.h"

class IntermediateStorage;
class FilePath;
class StorageProvider;

class IndexerWorkerServiceImpl final : public sourcetrail::IndexerWorkerService::Service {
public:
  explicit IndexerWorkerServiceImpl(std::shared_ptr<StorageProvider> storageProvider);
  ~IndexerWorkerServiceImpl() override = default;

  // Called by TaskFillIndexerCommandsQueue to populate the command queue
  void fillCommands(std::vector<sourcetrail::IndexerCommand> commands);

  // Number of commands still waiting to be pulled by a worker
  size_t pendingCommandCount();

  // Called by TaskBuildIndex when indexing is interrupted
  void setInterrupted(bool interrupted);

  // Called by TaskBuildIndex to wait until all in-flight work is finished
  std::vector<FilePath> drainAndGetCrashedFiles();

  // Called by TaskBuildIndex right before shutting down the gRPC server, so that
  // outstanding WatchInterrupt streams (which otherwise only exit on client
  // cancellation or an actual interrupt) return promptly instead of blocking
  // Server::Shutdown() forever.
  void requestShutdown();

  // Number of storages received (= files finished indexing)
  size_t getIndexedFileCount() const;

  // Number of files that have started indexing (monotonic; for progress display)
  size_t getStartedFileCount() const;

  // Currently active file paths (for progress dialog)
  std::vector<FilePath> getCurrentlyIndexedSourceFilePaths();

  // gRPC service methods
  grpc::Status PullCommand(grpc::ServerContext* ctx,
                           const sourcetrail::PullCommandRequest* req,
                           sourcetrail::PullCommandResponse* resp) override;

  grpc::Status PushIntermediateStorage(grpc::ServerContext* ctx,
                                       const sourcetrail::PushIntermediateStorageRequest* req,
                                       sourcetrail::PushIntermediateStorageResponse* resp) override;

  grpc::Status ReportStatus(grpc::ServerContext* ctx,
                            const sourcetrail::StatusReport* req,
                            sourcetrail::StatusReportResponse* resp) override;

  grpc::Status WatchInterrupt(grpc::ServerContext* ctx,
                              const sourcetrail::WatchInterruptRequest* req,
                              grpc::ServerWriter<sourcetrail::InterruptEvent>* writer) override;

private:
  std::shared_ptr<StorageProvider> mStorageProvider;

  std::deque<sourcetrail::IndexerCommand> mCommandQueue;
  std::mutex mCommandMutex;

  std::atomic<bool> mInterrupted{false};
  std::atomic<bool> mShuttingDown{false};

  // track in-flight files (for crash detection) + crashed files
  std::mutex mStatusMutex;
  std::vector<FilePath> mCrashedFiles;
  std::unordered_map<uint64_t, std::string> mCurrentFileByProcess;

  // broadcast interrupt to all WatchInterrupt streams
  std::atomic<size_t> mIndexedFileCount{0};
  std::atomic<size_t> mStartedFileCount{0};

  std::mutex mInterruptListenersMutex;
  std::vector<grpc::ServerWriter<sourcetrail::InterruptEvent>*> mInterruptListeners;
  void notifyInterruptListeners();
};
