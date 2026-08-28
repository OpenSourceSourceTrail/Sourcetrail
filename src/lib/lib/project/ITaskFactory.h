#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../scheduling/Task.h"
#include "../../scheduling/TaskDecoratorRepeat.h"
#include "../../scheduling/TaskReturnSuccessIf.h"
#include "../../scheduling/TaskSetValue.h"
#include "IndexerCommandType.h"

class DialogView;
class FilePath;
class IndexerCommandProvider;
class IndexerWorkerServiceImpl;
class PersistentStorage;
class Storage;
class StorageProvider;
class TaskBuildIndex;
class TaskCleanStorage;
class TaskExecuteCustomCommands;
class TaskFillIndexerCommandsQueue;
class TaskFinishParsing;
class TaskGroupSelector;
class TaskGroupSequence;
class TaskInjectStorage;
class TaskMergeStorages;
class TaskParseWrapper;
class TaskflowGroupParallel;
class TaskFindKeyOnBlackboard;
class TaskLambda;

// Abstracts construction of scheduling Task nodes for indexing pipelines, so
// callers depend on this interface rather than on every concrete Task type.
class ITaskFactory {
public:
  ITaskFactory() = default;
  virtual ~ITaskFactory() = default;
  ITaskFactory(const ITaskFactory&) = default;
  ITaskFactory(ITaskFactory&&) = default;
  ITaskFactory& operator=(const ITaskFactory&) = default;
  ITaskFactory& operator=(ITaskFactory&&) = default;

  virtual std::shared_ptr<TaskGroupSequence> createSequence() = 0;
  virtual std::shared_ptr<TaskGroupSelector> createSelector() = 0;
  virtual std::shared_ptr<TaskflowGroupParallel> createParallel() = 0;
  virtual std::shared_ptr<TaskFindKeyOnBlackboard> createFindKeyOnBlackboard(const std::string& valueName) = 0;
  virtual std::shared_ptr<TaskLambda> createLambda(std::function<void()> func) = 0;
  virtual std::shared_ptr<TaskDecoratorRepeat> createRepeat(TaskDecoratorRepeat::ConditionType condition,
                                                            Task::TaskState exitState,
                                                            size_t delayMS) = 0;

  virtual std::shared_ptr<TaskCleanStorage> createCleanStorage(std::weak_ptr<PersistentStorage> storage,
                                                               std::shared_ptr<DialogView> dialogView,
                                                               const std::vector<FilePath>& filePaths,
                                                               bool clearAllErrors) = 0;
  virtual std::shared_ptr<TaskParseWrapper> createParseWrapper(std::weak_ptr<PersistentStorage> storage,
                                                               std::shared_ptr<DialogView> dialogView) = 0;
  virtual std::shared_ptr<TaskFillIndexerCommandsQueue> createFillIndexerCommandsQueue(
      std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
      std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
      size_t maximumQueueSize) = 0;
  virtual std::shared_ptr<TaskBuildIndex> createBuildIndex(size_t processCount,
                                                           std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                                                           std::shared_ptr<StorageProvider> storageProvider,
                                                           std::shared_ptr<DialogView> dialogView,
                                                           std::string appUUID,
                                                           bool multiProcessIndexing,
                                                           IndexerCommandType commandType) = 0;
  virtual std::shared_ptr<TaskMergeStorages> createMergeStorages(std::shared_ptr<StorageProvider> storageProvider) = 0;
  virtual std::shared_ptr<TaskInjectStorage> createInjectStorage(std::shared_ptr<StorageProvider> storageProvider,
                                                                 std::weak_ptr<Storage> target) = 0;
  virtual std::shared_ptr<TaskExecuteCustomCommands> createExecuteCustomCommands(
      std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
      std::shared_ptr<PersistentStorage> storage,
      std::shared_ptr<DialogView> dialogView,
      size_t indexerThreadCount,
      FilePath projectDirectory) = 0;
  virtual std::shared_ptr<TaskFinishParsing> createFinishParsing(std::shared_ptr<PersistentStorage> storage,
                                                                 std::shared_ptr<DialogView> dialogView) = 0;

  // Template primitives cannot be virtual; these are simple non-polymorphic
  // factory methods kept on the interface for a single, consistent call site.
  template <typename T>
  std::shared_ptr<TaskSetValue<T>> createSetValue(const std::string& valueName, T value) {
    return std::make_shared<TaskSetValue<T>>(valueName, value);
  }

  template <typename T>
  std::shared_ptr<TaskReturnSuccessIf<T>> createReturnSuccessIfEquals(const std::string& valueName, T value) {
    return std::make_shared<TaskReturnSuccessIf<T>>(valueName, TaskReturnSuccessIf<T>::CONDITION_EQUALS, value);
  }
};
