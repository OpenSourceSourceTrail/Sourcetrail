#pragma once

#include "project/ITaskFactory.h"

// Default ITaskFactory implementation: builds real concrete Task instances.
class DefaultTaskFactory final : public ITaskFactory {
public:
  std::shared_ptr<TaskGroupSequence> createSequence() override;
  std::shared_ptr<TaskGroupSelector> createSelector() override;
  std::shared_ptr<TaskflowGroupParallel> createParallel() override;
  std::shared_ptr<TaskFindKeyOnBlackboard> createFindKeyOnBlackboard(const std::string& valueName) override;
  std::shared_ptr<TaskLambda> createLambda(std::function<void()> func) override;
  std::shared_ptr<TaskDecoratorRepeat> createRepeat(TaskDecoratorRepeat::ConditionType condition,
                                                    Task::TaskState exitState,
                                                    size_t delayMS) override;

  std::shared_ptr<TaskCleanStorage> createCleanStorage(std::weak_ptr<PersistentStorage> storage,
                                                       std::shared_ptr<DialogView> dialogView,
                                                       const std::vector<FilePath>& filePaths,
                                                       bool clearAllErrors) override;
  std::shared_ptr<TaskParseWrapper> createParseWrapper(std::weak_ptr<PersistentStorage> storage,
                                                       std::shared_ptr<DialogView> dialogView) override;
  std::shared_ptr<TaskFillIndexerCommandsQueue> createFillIndexerCommandsQueue(
      std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
      std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
      size_t maximumQueueSize) override;
  std::shared_ptr<TaskBuildIndex> createBuildIndex(size_t processCount,
                                                   std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                                                   std::shared_ptr<StorageProvider> storageProvider,
                                                   std::shared_ptr<DialogView> dialogView,
                                                   std::string appUUID,
                                                   bool multiProcessIndexing,
                                                   IndexerCommandType commandType) override;
  std::shared_ptr<TaskMergeStorages> createMergeStorages(std::shared_ptr<StorageProvider> storageProvider) override;
  std::shared_ptr<TaskInjectStorage> createInjectStorage(std::shared_ptr<StorageProvider> storageProvider,
                                                         std::weak_ptr<Storage> target) override;
  std::shared_ptr<TaskExecuteCustomCommands> createExecuteCustomCommands(std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                                                                         std::shared_ptr<PersistentStorage> storage,
                                                                         std::shared_ptr<DialogView> dialogView,
                                                                         size_t indexerThreadCount,
                                                                         FilePath projectDirectory) override;
  std::shared_ptr<TaskFinishParsing> createFinishParsing(std::shared_ptr<PersistentStorage> storage,
                                                         std::shared_ptr<DialogView> dialogView) override;
};
