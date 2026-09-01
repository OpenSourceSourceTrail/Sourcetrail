#pragma once
#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <condition_variable>
#include <grpcpp/grpcpp.h>

#include "indexer_worker.grpc.pb.h"

class IntermediateStorage;
class FilePath;
class StorageProvider;

class IndexerWorkerServiceImpl final : public sourcetrail::IndexerWorkerService::Service {
public:
  // How long PullCommand parks before telling an idle worker "nothing yet". Bounded so a parked
  // call cannot wedge Server::Shutdown(), long enough that waiting for work costs nothing.
  static constexpr std::chrono::milliseconds PullWait{1000};

  explicit IndexerWorkerServiceImpl(std::shared_ptr<StorageProvider> storageProvider);
  ~IndexerWorkerServiceImpl() override = default;

  // Called by TaskFillIndexerCommandsQueue to populate the command queue
  void fillCommands(std::vector<sourcetrail::IndexerCommand> commands);

  // Called by TaskFillIndexerCommandsQueue once its provider is drained: from here on an empty
  // queue means "no more work", which is what lets a worker exit. Commands still pending are
  // handed out first.
  void closeQueue();

  // Whether closeQueue() has been called. Authoritative and immediate, unlike the
  // "indexer_command_queue_stopped" blackboard flag TaskBuildIndex only re-reads every poll.
  bool isQueueClosed();

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
  std::condition_variable mCommandCv;
  bool mQueueClosed{false};    // guarded by mCommandMutex

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
