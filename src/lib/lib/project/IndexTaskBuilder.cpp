#include "project/IndexTaskBuilder.h"

#include <algorithm>
#include <utility>

#include "../../scheduling/TaskDecoratorRepeat.h"
#include "../../scheduling/TaskFindKeyOnBlackboard.h"
#include "../../scheduling/TaskflowGroupParallel.h"
#include "../../scheduling/TaskGroupSelector.h"
#include "../../scheduling/TaskGroupSequence.h"
#include "../../scheduling/TaskLambda.h"
#include "../../scheduling/TaskReturnSuccessIf.h"
#include "../../scheduling/TaskSetValue.h"
#include "component/TabId.h"
#include "component/view/DialogView.h"
#include "data/indexer/CombinedIndexerCommandProvider.h"
#include "data/indexer/grpc/IndexerWorkerServiceImpl.h"
#include "data/indexer/TaskBuildIndex.h"
#include "data/indexer/TaskExecuteCustomCommands.h"
#include "data/indexer/TaskFillIndexerCommandQueue.h"
#include "data/parser/TaskParseWrapper.h"
#include "data/storage/PersistentStorage.h"
#include "data/storage/StorageProvider.h"
#include "data/TaskCleanStorage.h"
#include "data/TaskFinishParsing.h"
#include "data/TaskInjectStorage.h"
#include "data/TaskMergeStorages.h"
#include "FilePath.h"
#include "project/ITaskFactory.h"
#include "project/SourceGroup.h"
#include "settings/IApplicationSettings.hpp"
#include "settings/LanguageType.h"
#include "settings/source_group/SourceGroupStatusType.h"
#include "type/indexing/MessageIndexingShowDialog.h"
#include "type/MessageRefresh.h"
#include "utility.h"
#include "utilityApp.h"
#include "utilityString.h"

namespace {
constexpr int DefaultIndexerThreadCount = 4;
constexpr size_t MinimumCommandQueueSize = 20;
constexpr size_t CommandsPerWorkerInQueue = 4;
/**
 * Concurrent merge branches. Each holds a taskflow worker for the whole run (it loops until another
 * branch flips a flag), so this cannot grow without also raising the executor's thread floor.
 */
constexpr int MergeBranchCount = 4;

// Single-language assumption (routing across mixed-language projects is a later increment):
// resolve the worker command type from the first enabled non-custom source group's language.
IndexerCommandType getStandardCommandType(const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups) {
  for(const std::shared_ptr<SourceGroup>& sourceGroup : sourceGroups) {
    if(sourceGroup->getStatus() != SOURCE_GROUP_STATUS_ENABLED) {
      continue;
    }
    if(sourceGroup->getLanguage() == LANGUAGE_JAVA) {
      return INDEXER_COMMAND_JAVA;
    }
    if(sourceGroup->getLanguage() == LANGUAGE_C || sourceGroup->getLanguage() == LANGUAGE_CPP) {
      return INDEXER_COMMAND_CXX;
    }
  }
  return INDEXER_COMMAND_UNKNOWN;
}

int getIndexerThreadCount() {
  int indexerThreadCount = IApplicationSettings::getInstanceRaw()->getIndexerThreadCount();
  if(indexerThreadCount <= 0) {
    indexerThreadCount = utility::getIdealThreadCount();
    if(indexerThreadCount <= 0) {
      indexerThreadCount = DefaultIndexerThreadCount;    // setting to some fallback value
    }
  }
  return indexerThreadCount;
}

}    // namespace

IndexTaskBuilder::IndexTaskBuilder(std::shared_ptr<ITaskFactory> taskFactory, std::string appUUID, Callbacks callbacks)
    : m_taskFactory(std::move(taskFactory)), m_appUUID(std::move(appUUID)), m_callbacks(std::move(callbacks)) {}

IndexTaskBuilder::IndexerCommandProviders IndexTaskBuilder::buildIndexerCommandProviders(
    const RefreshInfo& info, const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups) {
  IndexerCommandProviders providers;
  auto standard = std::make_unique<CombinedIndexerCommandProvider>();
  auto custom = std::make_unique<CombinedIndexerCommandProvider>();
  for(const std::shared_ptr<SourceGroup>& sourceGroup : sourceGroups) {
    if(SOURCE_GROUP_STATUS_ENABLED == sourceGroup->getStatus()) {
      if(SOURCE_GROUP_CUSTOM_COMMAND == sourceGroup->getType()) {
        custom->addProvider(sourceGroup->getIndexerCommandProvider(info));
      } else {
        standard->addProvider(sourceGroup->getIndexerCommandProvider(info));
      }
    }
  }
  providers.count = standard->size() + custom->size();
  providers.standard = std::move(standard);
  providers.custom = std::move(custom);
  return providers;
}

void IndexTaskBuilder::addCleanStorageTask(const std::shared_ptr<TaskGroupSequence>& sequence,
                                           const RefreshInfo& info,
                                           const std::shared_ptr<PersistentStorage>& tempStorage,
                                           const std::shared_ptr<DialogView>& dialogView) {
  if(RefreshMode::AllFiles != info.mode && (!info.filesToClear.empty() || !info.nonIndexedFilesToClear.empty())) {
    sequence->addTask(
        m_taskFactory->createCleanStorage(tempStorage,
                                          dialogView,
                                          utility::toVector(utility::concat(info.filesToClear, info.nonIndexedFilesToClear)),
                                          info.mode == RefreshMode::UpdatedAndIncompleteFiles));
  }
}

void IndexTaskBuilder::addBlackboardSetupTasks(const std::shared_ptr<TaskGroupSequence>& sequence,
                                               const RefreshInfo& info,
                                               size_t sourceFileCount) {
  sequence->addTask(m_taskFactory->createSetValue<bool>("shallow_indexing", info.shallow));
  sequence->addTask(m_taskFactory->createSetValue<int>("source_file_count", static_cast<int>(sourceFileCount)));
  sequence->addTask(m_taskFactory->createSetValue<int>("indexed_source_file_count", 0));
  sequence->addTask(m_taskFactory->createSetValue<bool>("interrupted_indexing", false));
  sequence->addTask(m_taskFactory->createSetValue<float>("index_time", 0.0F));
}

std::shared_ptr<Task> IndexTaskBuilder::makeBlockUntilTrue(const std::string& key, size_t delayMS) {
  return m_taskFactory->createRepeat(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, delayMS)
      ->addChildTask(m_taskFactory->createReturnSuccessIfEquals<bool>(key, false));
}

std::shared_ptr<Task> IndexTaskBuilder::makeAppThreadLambda(std::function<void()> func) {
  return m_taskFactory->createLambda(
      [this, func = std::move(func)]() { Task::dispatch(TabId::app(), m_taskFactory->createLambda(func)); });
}

void IndexTaskBuilder::addIndexerPipeline(const std::shared_ptr<TaskGroupSequence>& sequence,
                                          std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                                          const std::shared_ptr<PersistentStorage>& tempStorage,
                                          const std::shared_ptr<DialogView>& dialogView,
                                          const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups,
                                          int indexerThreadCount) {
  if(indexerCommandProvider->empty()) {
    dialogView->hideUnknownProgressDialog();
    return;
  }

  const int adjustedIndexerThreadCount = std::min<int>(indexerThreadCount, static_cast<int>(indexerCommandProvider->size()));

  auto storageProvider = std::make_shared<StorageProvider>();
  // Shared gRPC worker service: TaskBuildIndex hosts it, TaskFillIndexerCommandsQueue feeds it
  auto indexerWorkerService = std::make_shared<IndexerWorkerServiceImpl>(storageProvider);
  // add tasks for setting some variables on the blackboard that are used during indexing
  sequence->addTask(m_taskFactory->createSetValue<bool>("indexer_threads_started", false));
  sequence->addTask(m_taskFactory->createSetValue<bool>("indexer_threads_stopped", false));
  sequence->addTask(m_taskFactory->createSetValue<bool>("indexer_command_queue_started", false));
  sequence->addTask(m_taskFactory->createSetValue<bool>("indexer_command_queue_stopped", false));

  auto preIndexTasks = m_taskFactory->createSequence();
  sequence->addTask(preIndexTasks);
  for(const std::shared_ptr<SourceGroup>& sourceGroup : sourceGroups) {
    if(sourceGroup->getStatus() == SOURCE_GROUP_STATUS_ENABLED) {
      preIndexTasks->addTask(sourceGroup->getPreIndexTask(storageProvider, dialogView));
    }
  }

  auto taskParserWrapper = m_taskFactory->createParseWrapper(tempStorage, dialogView);
  sequence->addTask(taskParserWrapper);

  auto taskParallelIndexing = m_taskFactory->createParallel();
  taskParserWrapper->setTask(taskParallelIndexing);

  // add task for refilling the indexer command queue
  // Deep enough that every worker can hold a command and still find the next one waiting; a
  // fixed size smaller than the worker count leaves most of the pool idle.
  const size_t commandQueueSize = std::max<size_t>(
      MinimumCommandQueueSize, static_cast<size_t>(adjustedIndexerThreadCount) * CommandsPerWorkerInQueue);
  taskParallelIndexing->addTask(
      m_taskFactory->createFillIndexerCommandsQueue(indexerWorkerService, std::move(indexerCommandProvider), commandQueueSize));

  // add task for indexing
  taskParallelIndexing->addChildTasks(m_taskFactory->createSequence()->addChildTasks(
      // block until there are indexer commands to process
      makeBlockUntilTrue("indexer_command_queue_started", 25),
      m_taskFactory->createBuildIndex(static_cast<size_t>(adjustedIndexerThreadCount),
                                      indexerWorkerService,
                                      storageProvider,
                                      dialogView,
                                      m_appUUID,
                                      getStandardCommandType(sourceGroups))));

  // add tasks for merging the intermediate storages
  // Merging is what keeps the injector's work down -- dropping it entirely costs ~60% more wall
  // time -- but one merger cannot keep up with the worker pool, so run several. Each merge takes
  // its own pair out of the provider under that provider's mutex, so they do not contend further.
  for(int mergeBranch = 0; mergeBranch < MergeBranchCount; ++mergeBranch) {
    taskParallelIndexing->addTask(m_taskFactory->createSequence()->addChildTasks(
        // block until there are indexers running
        makeBlockUntilTrue("indexer_threads_started", 25),
        // merge until all indexers stopped and nothing left to merge
        m_taskFactory->createRepeat(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 0)
            ->addChildTask(m_taskFactory->createSelector()->addChildTasks(
                m_taskFactory->createMergeStorages(storageProvider),
                m_taskFactory->createReturnSuccessIfEquals<bool>("indexer_threads_stopped", false)))));
  }

  // add task for injecting the intermediate storages into the persistent storage
  taskParallelIndexing->addTask(m_taskFactory->createSequence()->addChildTasks(
      // block until there are indexers running
      makeBlockUntilTrue("indexer_threads_started", 25),
      m_taskFactory->createRepeat(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 0)
          ->addChildTask(m_taskFactory->createSelector()->addChildTasks(
              m_taskFactory->createInjectStorage(storageProvider, tempStorage),
              // continuing when indexers still running, even if there are no storages right now.
              m_taskFactory->createReturnSuccessIfEquals<bool>("indexer_threads_stopped", false)))));

  // add task that notifies the user of what's going on
  sequence->addTask(    // we don't need to hide this dialog again, because it's
                        // overridden by other dialogs later on.
      m_taskFactory->createLambda(
          [dialogView]() { dialogView->showUnknownProgressDialog(L"Finish Indexing", L"Saving\nRemaining Data"); }));

  // add task that injects the remaining intermediate storages into the persistent storage
  sequence->addTask(m_taskFactory->createRepeat(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 0)
                        ->addChildTask(m_taskFactory->createInjectStorage(storageProvider, tempStorage)));
}

void IndexTaskBuilder::addCustomCommandTask(const std::shared_ptr<TaskGroupSequence>& sequence,
                                            std::unique_ptr<IndexerCommandProvider> customIndexerCommandProvider,
                                            const std::shared_ptr<PersistentStorage>& tempStorage,
                                            const std::shared_ptr<DialogView>& dialogView,
                                            const FilePath& projectDirectory,
                                            int indexerThreadCount) {
  if(customIndexerCommandProvider->empty()) {
    return;
  }

  const int adjustedIndexerThreadCount = std::min<int>(indexerThreadCount, static_cast<int>(customIndexerCommandProvider->size()));

  sequence->addTask(m_taskFactory->createExecuteCustomCommands(std::move(customIndexerCommandProvider),
                                                               tempStorage,
                                                               dialogView,
                                                               static_cast<size_t>(adjustedIndexerThreadCount),
                                                               projectDirectory));
}

void IndexTaskBuilder::addFinalizationTasks(const std::shared_ptr<TaskGroupSequence>& sequence,
                                            const std::shared_ptr<PersistentStorage>& tempStorage,
                                            const std::shared_ptr<DialogView>& dialogView) {
  sequence->addTask(m_taskFactory->createFinishParsing(tempStorage, dialogView));

  sequence->addTask(m_taskFactory->createSelector()->addChildTasks(
      m_taskFactory->createSequence()->addChildTasks(
          m_taskFactory->createFindKeyOnBlackboard("keep_database"),
          makeAppThreadLambda([this, dialogView]() { m_callbacks.onKeepDatabase(dialogView); })),
      m_taskFactory->createSequence()->addChildTasks(m_taskFactory->createFindKeyOnBlackboard("discard_database"),
                                                     makeAppThreadLambda([this]() { m_callbacks.onDiscardDatabase(); }))));

  sequence->addTask(m_taskFactory->createLambda([this]() { m_callbacks.onIndexingFinished(); }));

  sequence->addTask(m_taskFactory->createSelector()->addChildTasks(
      m_taskFactory->createSequence()->addChildTasks(
          m_taskFactory->createFindKeyOnBlackboard("refresh_database"), makeAppThreadLambda([]() {
            MessageIndexingShowDialog().dispatch();
            MessageRefresh().refreshAll().dispatch();
          })),
      m_taskFactory->createSequence()->addChildTasks(makeAppThreadLambda([]() {}))));
}

std::shared_ptr<TaskGroupSequence> IndexTaskBuilder::createIndexTasks(RefreshInfo info,
                                                                      std::shared_ptr<DialogView> dialogView,
                                                                      std::shared_ptr<PersistentStorage> tempStorage,
                                                                      const std::vector<std::shared_ptr<SourceGroup>>& sourceGroups,
                                                                      const FilePath& projectDirectory,
                                                                      size_t& sourceFileCount) {
  auto taskSequential = m_taskFactory->createSequence();

  auto indexingWork = m_taskFactory->createSequence();

  addCleanStorageTask(indexingWork, info, tempStorage, dialogView);

  IndexerCommandProviders providers = buildIndexerCommandProviders(info, sourceGroups);
  sourceFileCount = providers.count;

  addBlackboardSetupTasks(indexingWork, info, sourceFileCount);

  const int indexerThreadCount = getIndexerThreadCount();

  addIndexerPipeline(indexingWork, std::move(providers.standard), tempStorage, dialogView, sourceGroups, indexerThreadCount);
  addCustomCommandTask(indexingWork, std::move(providers.custom), tempStorage, dialogView, projectDirectory, indexerThreadCount);

  // A failing child latches TaskGroupSequence into permanent failure, so the finalization tasks
  // below would never run -- and onIndexingFinished is the only thing that clears the project's
  // refresh stage. Without this, one failed run (a worker crash loop, an exception) silently makes
  // every later refresh a no-op until the process restarts. The selector falls through to the
  // no-op on failure, so finalization runs either way, exactly as it already does after an
  // interrupt.
  taskSequential->addTask(m_taskFactory->createSelector()->addChildTasks(indexingWork, m_taskFactory->createLambda([]() {})));

  addFinalizationTasks(taskSequential, tempStorage, dialogView);

  taskSequential->setIsBackgroundTask(true);
  return taskSequential;
}
