#pragma once
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <grpcpp/server.h>

#include "data/indexer/grpc/IndexerWorkerServiceImpl.h"
#include "data/indexer/IndexerCommandType.h"
#include "MessageListener.h"
#include "Task.h"
#include "type/indexing/MessageIndexingInterrupted.h"

class DialogView;
class StorageProvider;
class IndexerCommandList;

class TaskBuildIndex
    : public Task
    , public MessageListener<MessageIndexingInterrupted> {
public:
  TaskBuildIndex(size_t processCount,
                 std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                 std::shared_ptr<StorageProvider> storageProvider,
                 std::shared_ptr<DialogView> dialogView,
                 std::string appUUID,
                 IndexerCommandType commandType);

  /**
   * Does this exit look like a worker that died on startup rather than one that finished a batch?
   *
   * A worker that pulls no work and exits non-zero within a moment is not making progress, so
   * respawning it immediately is a busy loop. A slow non-zero exit did real work first and is retried.
   */
  [[nodiscard]] static bool isCrashLoopExit(int exitCode, std::chrono::milliseconds ranFor);

  /** Consecutive crash-loop exits after which a worker is abandoned. */
  static constexpr int MaxConsecutiveWorkerFailures = 3;

protected:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void terminate() override;

  void handleMessage(MessageIndexingInterrupted* message) override;

  void runIndexerProcess(int processId, const std::wstring& logFilePath);
  void updateIndexingDialog(const std::shared_ptr<Blackboard>& blackboard, const std::vector<FilePath>& sourcePaths);

  static const std::wstring sProcessName;

  std::shared_ptr<IndexerCommandList> mIndexerCommandList;
  std::shared_ptr<StorageProvider> mStorageProvider;
  std::shared_ptr<DialogView> mDialogView;
  const std::string mAppUUID;
  IndexerCommandType mCommandType;

  std::shared_ptr<IndexerWorkerServiceImpl> mIndexerWorkerService;
  std::unique_ptr<grpc::Server> mGrpcServer;
  int mEnginePort{0};

  bool mIndexerCommandQueueStopped = false;
  size_t mProcessCount;
  bool mInterrupted = false;
  size_t mLastReportedIndexedCount = 0;

  // store as plain pointers to avoid deallocation issues when closing app during indexing
  std::vector<std::unique_ptr<std::thread>> mProcessThreads;

  size_t mRunningThreadCount = 0;
  std::mutex mRunningThreadCountMutex;
};
