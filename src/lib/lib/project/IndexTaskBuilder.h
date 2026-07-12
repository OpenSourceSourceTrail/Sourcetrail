#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "RefreshInfo.h"

class DialogView;
class FilePath;
class IndexerCommandProvider;
class ITaskFactory;
class PersistentStorage;
class SourceGroup;
class Task;
class TaskGroupSequence;

// Builds the Task tree that performs an indexing run for a Project, without
// depending on Project itself. Project-specific actions (persisting the keep/
// discard decision, notifying that indexing finished) are supplied as callbacks.
class IndexTaskBuilder {
public:
  struct Callbacks {
    std::function<void(std::shared_ptr<DialogView>)> onKeepDatabase;
    std::function<void()> onDiscardDatabase;
    std::function<void()> onIndexingFinished;
    std::function<bool()> hasCxxSourceGroup;
    std::function<bool()> hasJavaSourceGroup;
  };

  IndexTaskBuilder(std::shared_ptr<ITaskFactory> taskFactory, std::string appUUID, Callbacks callbacks);

  std::shared_ptr<TaskGroupSequence> createIndexTasks(RefreshInfo info,
                                                       std::shared_ptr<DialogView> dialogView,
                                                       std::shared_ptr<PersistentStorage> tempStorage,
                                                       const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups,
                                                       const FilePath& projectDirectory,
                                                       size_t& sourceFileCount);

private:
  // Holds the two IndexerCommandProviders (standard + custom-command source groups)
  // collected from the currently enabled source groups.
  struct IndexerCommandProviders {
    std::unique_ptr<IndexerCommandProvider> standard;
    std::unique_ptr<IndexerCommandProvider> custom;
    size_t count = 0;
  };

  static IndexerCommandProviders buildIndexerCommandProviders(const RefreshInfo& info,
                                                               const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups);

  void addCleanStorageTask(const std::shared_ptr<TaskGroupSequence>& sequence,
                           const RefreshInfo& info,
                           const std::shared_ptr<PersistentStorage>& tempStorage,
                           const std::shared_ptr<DialogView>& dialogView);

  void addBlackboardSetupTasks(const std::shared_ptr<TaskGroupSequence>& sequence, const RefreshInfo& info, size_t sourceFileCount);

  void addIndexerPipeline(const std::shared_ptr<TaskGroupSequence>& sequence,
                          std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                          const std::shared_ptr<PersistentStorage>& tempStorage,
                          const std::shared_ptr<DialogView>& dialogView,
                          const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups,
                          int indexerThreadCount);

  void addCustomCommandTask(const std::shared_ptr<TaskGroupSequence>& sequence,
                            std::unique_ptr<IndexerCommandProvider> customIndexerCommandProvider,
                            const std::shared_ptr<PersistentStorage>& tempStorage,
                            const std::shared_ptr<DialogView>& dialogView,
                            const FilePath& projectDirectory,
                            int indexerThreadCount);

  void addFinalizationTasks(const std::shared_ptr<TaskGroupSequence>& sequence,
                            const std::shared_ptr<PersistentStorage>& tempStorage,
                            const std::shared_ptr<DialogView>& dialogView);

  // Blocks (as long as delayMS repeats allow) until the bool blackboard value at
  // `key` becomes true.
  std::shared_ptr<Task> makeBlockUntilTrue(const std::string& key, size_t delayMS);

  // Wraps `func` so it runs on the app tab's task scheduler, dispatched from
  // within another task's execution.
  std::shared_ptr<Task> makeAppThreadLambda(std::function<void()> func);

  std::shared_ptr<ITaskFactory> m_taskFactory;
  std::string m_appUUID;
  Callbacks m_callbacks;
};
