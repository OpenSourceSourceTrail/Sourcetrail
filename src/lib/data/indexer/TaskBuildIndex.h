#pragma once
#include <thread>

#include "InterprocessIndexerCommandManager.h"
#include "InterprocessIndexingStatusManager.h"
#include "InterprocessIntermediateStorageManager.h"
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
                 std::shared_ptr<StorageProvider> storageProvider,
                 std::shared_ptr<DialogView> dialogView,
                 std::string appUUID,
                 bool multiProcessIndexing);

protected:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void terminate() override;

  void handleMessage(MessageIndexingInterrupted* message) override;

  void runIndexerProcess(int processId, const std::wstring& logFilePath);
  void runIndexerThread(int processId);
  bool fetchIntermediateStorages(const std::shared_ptr<Blackboard>& blackboard);
  void updateIndexingDialog(const std::shared_ptr<Blackboard>& blackboard, const std::vector<FilePath>& sourcePaths);

  static const std::wstring sProcessName;

  std::shared_ptr<IndexerCommandList> mIndexerCommandList;
  std::shared_ptr<StorageProvider> mStorageProvider;
  std::shared_ptr<DialogView> mDialogView;
  const std::string mAppUUID;
  bool mMultiProcessIndexing;

  InterprocessIndexingStatusManager mInterprocessIndexingStatusManager;
  bool mIndexerCommandQueueStopped = false;
  size_t mProcessCount;
  bool mInterrupted = false;
  size_t mIndexingFileCount = 0;

  // store as plain pointers to avoid deallocation issues when closing app during indexing
  std::vector<std::unique_ptr<std::thread>> mProcessThreads;
  std::vector<std::shared_ptr<InterprocessIntermediateStorageManager>> mInterprocessIntermediateStorageManagers;

  size_t mRunningThreadCount = 0;
  std::mutex mRunningThreadCountMutex;
};
