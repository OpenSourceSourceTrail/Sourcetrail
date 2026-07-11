#pragma once
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <grpcpp/server.h>

#include "IndexerCommandType.h"
#include "IndexerWorkerServiceImpl.h"
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
                 bool multiProcessIndexing,
                 IndexerCommandType commandType);

protected:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void terminate() override;

  void handleMessage(MessageIndexingInterrupted* message) override;

  void runIndexerProcess(int processId, const std::wstring& logFilePath);
  void runIndexerThread(int processId);
  void updateIndexingDialog(const std::shared_ptr<Blackboard>& blackboard, const std::vector<FilePath>& sourcePaths);

  static const std::wstring sProcessName;

  std::shared_ptr<IndexerCommandList> mIndexerCommandList;
  std::shared_ptr<StorageProvider> mStorageProvider;
  std::shared_ptr<DialogView> mDialogView;
  const std::string mAppUUID;
  bool mMultiProcessIndexing;
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
