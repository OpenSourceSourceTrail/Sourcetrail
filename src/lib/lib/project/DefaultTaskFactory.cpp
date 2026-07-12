#include "DefaultTaskFactory.h"

#include "../../scheduling/TaskFindKeyOnBlackboard.h"
#include "../../scheduling/TaskGroupSelector.h"
#include "../../scheduling/TaskGroupSequence.h"
#include "../../scheduling/TaskLambda.h"
#include "../../scheduling/TaskflowGroupParallel.h"
#include "../data/TaskCleanStorage.h"
#include "../data/TaskFinishParsing.h"
#include "../data/TaskInjectStorage.h"
#include "../data/TaskMergeStorages.h"
#include "../data/indexer/TaskBuildIndex.h"
#include "../data/indexer/TaskExecuteCustomCommands.h"
#include "../data/indexer/IndexerCommandProvider.h"
#include "../data/indexer/TaskFillIndexerCommandQueue.h"
#include "../data/parser/TaskParseWrapper.h"

std::shared_ptr<TaskGroupSequence> DefaultTaskFactory::createSequence() {
  return std::make_shared<TaskGroupSequence>();
}

std::shared_ptr<TaskGroupSelector> DefaultTaskFactory::createSelector() {
  return std::make_shared<TaskGroupSelector>();
}

std::shared_ptr<TaskflowGroupParallel> DefaultTaskFactory::createParallel() {
  return std::make_shared<TaskflowGroupParallel>();
}

std::shared_ptr<TaskFindKeyOnBlackboard> DefaultTaskFactory::createFindKeyOnBlackboard(const std::string& valueName) {
  return std::make_shared<TaskFindKeyOnBlackboard>(valueName);
}

std::shared_ptr<TaskLambda> DefaultTaskFactory::createLambda(std::function<void()> func) {
  return std::make_shared<TaskLambda>(std::move(func));
}

std::shared_ptr<TaskDecoratorRepeat> DefaultTaskFactory::createRepeat(TaskDecoratorRepeat::ConditionType condition,
                                                                       Task::TaskState exitState,
                                                                       size_t delayMS) {
  return std::make_shared<TaskDecoratorRepeat>(condition, exitState, delayMS);
}

std::shared_ptr<TaskCleanStorage> DefaultTaskFactory::createCleanStorage(std::weak_ptr<PersistentStorage> storage,
                                                                          std::shared_ptr<DialogView> dialogView,
                                                                          const std::vector<FilePath>& filePaths,
                                                                          bool clearAllErrors) {
  return std::make_shared<TaskCleanStorage>(std::move(storage), std::move(dialogView), filePaths, clearAllErrors);
}

std::shared_ptr<TaskParseWrapper> DefaultTaskFactory::createParseWrapper(std::weak_ptr<PersistentStorage> storage,
                                                                          std::shared_ptr<DialogView> dialogView) {
  return std::make_shared<TaskParseWrapper>(std::move(storage), std::move(dialogView));
}

std::shared_ptr<TaskFillIndexerCommandsQueue> DefaultTaskFactory::createFillIndexerCommandsQueue(
    std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
    std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
    size_t maximumQueueSize) {
  return std::make_shared<TaskFillIndexerCommandsQueue>(
      std::move(indexerWorkerService), std::move(indexerCommandProvider), maximumQueueSize);
}

std::shared_ptr<TaskBuildIndex> DefaultTaskFactory::createBuildIndex(
    size_t processCount,
    std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
    std::shared_ptr<StorageProvider> storageProvider,
    std::shared_ptr<DialogView> dialogView,
    std::string appUUID,
    bool multiProcessIndexing,
    IndexerCommandType commandType) {
  return std::make_shared<TaskBuildIndex>(processCount,
                                           std::move(indexerWorkerService),
                                           std::move(storageProvider),
                                           std::move(dialogView),
                                           std::move(appUUID),
                                           multiProcessIndexing,
                                           commandType);
}

std::shared_ptr<TaskMergeStorages> DefaultTaskFactory::createMergeStorages(std::shared_ptr<StorageProvider> storageProvider) {
  return std::make_shared<TaskMergeStorages>(std::move(storageProvider));
}

std::shared_ptr<TaskInjectStorage> DefaultTaskFactory::createInjectStorage(std::shared_ptr<StorageProvider> storageProvider,
                                                                            std::weak_ptr<Storage> target) {
  return std::make_shared<TaskInjectStorage>(std::move(storageProvider), std::move(target));
}

std::shared_ptr<TaskExecuteCustomCommands> DefaultTaskFactory::createExecuteCustomCommands(
    std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
    std::shared_ptr<PersistentStorage> storage,
    std::shared_ptr<DialogView> dialogView,
    size_t indexerThreadCount,
    FilePath projectDirectory) {
  return std::make_shared<TaskExecuteCustomCommands>(std::move(indexerCommandProvider),
                                                      std::move(storage),
                                                      std::move(dialogView),
                                                      indexerThreadCount,
                                                      std::move(projectDirectory));
}

std::shared_ptr<TaskFinishParsing> DefaultTaskFactory::createFinishParsing(std::shared_ptr<PersistentStorage> storage,
                                                                            std::shared_ptr<DialogView> dialogView) {
  return std::make_shared<TaskFinishParsing>(std::move(storage), std::move(dialogView));
}
