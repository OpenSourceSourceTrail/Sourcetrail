#pragma once
#include <set>
#include <vector>

#include "ErrorCountInfo.h"
#include "FilePath.h"
#include "MessageListener.h"
#include "Task.h"
#include "TimeStamp.h"
#include "type/indexing/MessageIndexingInterrupted.h"

class DialogView;
class IndexerCommandCustom;
class IndexerCommandProvider;
class PersistentStorage;

class TaskExecuteCustomCommands
    : public Task
    , public MessageListener<MessageIndexingInterrupted> {
public:
  TaskExecuteCustomCommands(std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                            std::shared_ptr<PersistentStorage> storage,
                            std::shared_ptr<DialogView> dialogView,
                            size_t indexerThreadCount,
                            FilePath projectDirectory);

private:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;

  void handleMessage(MessageIndexingInterrupted* message) override;

  void executeParallelIndexerCommands(int threadId, const std::shared_ptr<Blackboard>& blackboard);
  void runIndexerCommand(const std::shared_ptr<IndexerCommandCustom>& indexerCommand,
                         const std::shared_ptr<Blackboard>& blackboard,
                         const std::shared_ptr<PersistentStorage>& storage);

  std::unique_ptr<IndexerCommandProvider> mIndexerCommandProvider;
  std::shared_ptr<PersistentStorage> mStorage;
  std::shared_ptr<DialogView> mDialogView;
  const size_t mIndexerThreadCount;
  const FilePath mProjectDirectory;

  TimeStamp mStart;
  bool mInterrupted = false;
  size_t mIndexerCommandCount;
  std::vector<std::shared_ptr<IndexerCommandCustom>> mSerialCommands;
  std::vector<std::shared_ptr<IndexerCommandCustom>> mParallelCommands;
  std::mutex mParallelCommandsMutex;
  ErrorCountInfo mErrorCount;
  std::mutex mErrorCountMutex;
  FilePath mTargetDatabaseFilePath;
  std::set<FilePath> mSourceDatabaseFilePaths;
  std::mutex mSourceDatabaseFilePathsMutex;
};
