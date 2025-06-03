#include "Project.h"

#include <cassert>
#include <tuple>
#include <utility>

#include <fmt/format.h>

#include <range/v3/algorithm/any_of.hpp>

#include "DialogView.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "logging.h"
#include "PersistentStorage.h"
#include "ProjectSettings.h"
#include "StorageCache.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/MessageStatus.h"
#include "utilityApp.h"

#if !defined(SOURCETRAIL_WASM)
#  include "../../scheduling/TaskDecoratorRepeat.h"
#  include "../../scheduling/TaskFindKeyOnBlackboard.h"
#  include "../../scheduling/TaskGroupParallel.h"
#  include "../../scheduling/TaskGroupSelector.h"
#  include "../../scheduling/TaskGroupSequence.h"
#  include "../../scheduling/TaskLambda.h"
#  include "../../scheduling/TaskReturnSuccessIf.h"
#  include "../../scheduling/TaskSetValue.h"
#  include "CombinedIndexerCommandProvider.h"
#  include "FileSystem.h"
#  include "RefreshInfoGenerator.h"
#  include "SourceGroup.h"
#  include "SourceGroupFactory.h"
#  include "SourceGroupStatusType.h"
#  include "StorageProvider.h"
#  include "TabId.h"
#  include "TaskBuildIndex.h"
#  include "TaskCleanStorage.h"
#  include "TaskExecuteCustomCommands.h"
#  include "TaskFillIndexerCommandQueue.h"
#  include "TaskFinishParsing.h"
#  include "TaskInjectStorage.h"
#  include "TaskMergeStorages.h"
#  include "TaskParseWrapper.h"
#  include "TextAccess.h"
#  include "type/error/MessageErrorCountClear.h"
#  include "type/indexing/MessageIndexingShowDialog.h"
#  include "type/indexing/MessageIndexingStarted.h"
#  include "type/indexing/MessageIndexingStatus.h"
#  include "type/MessageRefresh.h"
#  include "utility.h"
#  include "utilityString.h"
#endif

namespace {
#if !defined(SOURCETRAIL_WASM)
constexpr int DefaultIndexerThreadCount = 4;

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
#endif
}    // namespace

Project::Project(std::shared_ptr<ProjectSettings> settings, StorageCache* storageCache, std::string appUUID, bool hasGUI) noexcept
    : m_settings(std::move(settings)), m_storageCache(storageCache), m_appUUID(std::move(appUUID)), m_hasGUI(hasGUI) {}

Project::~Project() noexcept = default;

FilePath Project::getProjectSettingsFilePath() const {
  return m_settings->getFilePath();
}

std::string Project::getDescription() const {
  return m_settings->getDescription();
}

bool Project::isLoaded() const {
  switch(m_state) {
  case ProjectStateType::EMPTY:
  case ProjectStateType::LOADED:
  case ProjectStateType::OUTDATED:
  case ProjectStateType::NEEDS_MIGRATION:
    return true;

  default:
    break;
  }

  return false;
}

bool Project::isIndexing() const {
  return m_refreshStage == RefreshStageType::INDEXING;
}

bool Project::settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const {
  return m_settings->equalsExceptNameAndLocation(otherSettings);
}

void Project::setStateOutdated() {
  if(ProjectStateType::LOADED == m_state) {
    m_state = ProjectStateType::OUTDATED;
  }
}

void Project::load([[maybe_unused]] const std::shared_ptr<DialogView>& dialogView) {
  if(m_refreshStage != RefreshStageType::NONE) {
    MessageStatus(L"Cannot load another project while indexing.", true, false).dispatch();
    return;
  }

  m_storageCache->clear();
  // TODO: check if this is really required.
  m_storageCache->setSubject(std::weak_ptr<StorageAccess>());

  if(!m_settings->reload()) {
    return;
  }

  // clang-format off
  const auto projectSettingsPath = m_settings->getFilePath();
  const auto dbPath              = m_settings->getDBFilePath();
  const auto tempDbPath          = m_settings->getTempDBFilePath();
  const auto bookmarkDbPath      = m_settings->getBookmarkDBFilePath();
  // clang-format on

  {
#if !defined(SOURCETRAIL_WASM)
    if(tempDbPath.exists()) {
      if(dbPath.exists()) {
        if(dialogView->confirm(L"Sourcetrail has been closed unexpectedly while indexing this project. "
                               L"You can either choose to keep "
                               L"the data that has already been indexed or discard that data and restore "
                               L"the state of your project "
                               L"before indexing?",
                               {L"Keep and Continue", L"Discard and Restore"}) == 0) {
          LOG_INFO("Switching to temporary indexing data on user's decision");
          if(!swapToTempStorageFile(dbPath, tempDbPath, dialogView)) {
            m_state = ProjectStateType::NOT_LOADED;
            MessageStatus(L"Unable to load project", true, false).dispatch();
            return;
          }
        } else {
          LOG_INFO("Discarding temporary indexing data on user's decision");
          FileSystem::remove(tempDbPath);
        }
      } else {
        LOG_INFO(
            "Switching to temporary indexing data because no other persistent data was "
            "found");
        FileSystem::rename(tempDbPath, dbPath);
      }
    }
#else
    MessageStatus{L"Sourcetrail has been closed unexpectedly while indexing this project.", true}.dispatch();
#endif
  }

  m_storage = std::make_shared<PersistentStorage>(dbPath, bookmarkDbPath);

  bool canLoad = false;

  if(m_settings->needMigration()) {
    m_state = ProjectStateType::NEEDS_MIGRATION;

    if(!m_storage->isEmpty() && !m_storage->isIncompatible()) {
      canLoad = true;
    }
  } else if(m_storage->isEmpty()) {
    m_state = ProjectStateType::EMPTY;
  } else if(m_storage->isIncompatible()) {
    m_state = ProjectStateType::OUTVERSIONED;
  } else {
    ProjectSettings storedSettings;
    if(storedSettings.loadFromString(m_storage->getProjectSettingsText()) &&
       m_settings->equalsExceptNameAndLocation(storedSettings)) {
      m_state = ProjectStateType::LOADED;
      canLoad = true;
    } else {
      m_state = ProjectStateType::OUTDATED;
      canLoad = true;
    }
  }

  try {
    m_storage->setup();
  } catch(...) {
    LOG_ERROR("Exception has been encountered while loading the project.");

    canLoad = false;
    m_state = ProjectStateType::DB_CORRUPTED;
  }

#if !defined(SOURCETRAIL_WASM)
  m_sourceGroups = SourceGroupFactory::getInstance()->createSourceGroups(m_settings->getAllSourceGroupSettings());
#endif

  if(canLoad) {
    m_storage->setMode(SqliteIndexStorage::STORAGE_MODE_READ);
    m_storage->buildCaches();
    m_storageCache->setSubject(m_storage);

    if(m_hasGUI) {
      MessageIndexingFinished{}.dispatch();
    }
    MessageStatus{L"Finished Loading", false, false}.dispatch();
  } else {
#if defined(SOURCETRAIL_WASM)
    MessageStatus{fmt::format(L"Project could not be loaded. state is {}.", static_cast<int>(m_state)), true, false}.dispatch();
#else
    switch(m_state) {
    case ProjectStateType::NEEDS_MIGRATION:
      MessageStatus(
          L"Project could not be loaded and needs to be re-indexed after automatic migration "
          L"to latest "
          "version.",
          false,
          false)
          .dispatch();
      break;
    case ProjectStateType::EMPTY:
      MessageStatus(
          L"Project could not load any symbols because the index database is empty. Please "
          L"re-index the "
          "project.",
          false,
          false)
          .dispatch();
      break;
    case ProjectStateType::OUTVERSIONED:
      MessageStatus(
          L"Project could not be loaded because the indexed data format is incompatible to "
          L"the current "
          "version of Sourcetrail. Please re-index the project.",
          false,
          false)
          .dispatch();
      break;
    default:
      MessageStatus(L"Project could not be loaded.", false, false).dispatch();
    }
#endif
  }

#if !defined(SOURCETRAIL_WASM)
  if(m_state != ProjectStateType::LOADED && m_hasGUI) {
    MessageRefresh{}.dispatch();
  }
#endif
}

#if !defined(SOURCETRAIL_WASM)
void Project::refresh(std::shared_ptr<DialogView> dialogView, RefreshMode refreshMode, bool shallowIndexingRequested) {
  if(m_refreshStage != RefreshStageType::NONE) {
    return;
  }

  if(m_state == ProjectStateType::NOT_LOADED) {
    return;
  }

  bool needsFullRefresh = false;
  bool fullRefresh = false;
  std::wstring question;

  switch(m_state) {
  case ProjectStateType::EMPTY:
    needsFullRefresh = true;
    break;

  case ProjectStateType::LOADED:
    break;

  case ProjectStateType::OUTDATED:
    question =
        L"The project file was changed after the last indexing. The project needs to get fully "
        L"reindexed to "
        L"reflect the current project state. Alternatively you can also choose to just reindex "
        L"updated or "
        L"incomplete files. Do you want to reindex the project?";
    fullRefresh = true;
    break;

  case ProjectStateType::OUTVERSIONED:
    question =
        L"This project was indexed with a different version of Sourcetrail. It needs to be "
        L"fully reindexed to "
        L"be used with this version of Sourcetrail. Do you want to reindex the project?";
    needsFullRefresh = true;
    break;

  case ProjectStateType::NEEDS_MIGRATION:
    question =
        L"This project was created with a different version and uses an old project file "
        L"format. "
        L"The project can still be opened and used with this version, but needs to be fully "
        L"reindexed. "
        L"Do you want Sourcetrail to update the project file and reindex the project?";
    needsFullRefresh = true;
    break;

  case ProjectStateType::DB_CORRUPTED:
    question =
        L"There was a problem loading the index of this project. The project needs to get "
        L"fully reindexed. "
        L"Do you want to reindex the project?";
    needsFullRefresh = true;
    break;

  default:
    break;
  }

  if(question.size() && m_hasGUI) {
    if(dialogView->confirm(question, {L"Reindex", L"Cancel"}) == 1) {
      return;
    }
  }

  if(IApplicationSettings::getInstanceRaw()->getLoggingEnabled() &&
     IApplicationSettings::getInstanceRaw()->getVerboseIndexerLoggingEnabled() && m_hasGUI) {
    if(dialogView->confirm(L"Warning: You are about to index your project with the \"verbose indexer "
                           L"logging\" setting "
                           L"enabled. This will cause a significant slowdown in indexing performance. Do you "
                           L"want to proceed?",
                           {L"Yes", L"No"}) == 1) {
      return;
    }
  }

  m_refreshStage = RefreshStageType::REFRESHING;

  if(m_state == ProjectStateType::NEEDS_MIGRATION) {
    m_settings->migrate();
  }

  m_settings->reload();

  m_sourceGroups = SourceGroupFactory::getInstance()->createSourceGroups(m_settings->getAllSourceGroupSettings());
  for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
    if(sourceGroup->getStatus() == SOURCE_GROUP_STATUS_ENABLED && !sourceGroup->prepareIndexing()) {
      m_refreshStage = RefreshStageType::NONE;
      return;
    }
  }

  if(needsFullRefresh || fullRefresh) {
    refreshMode = RefreshMode::AllFiles;
  } else if(refreshMode == RefreshMode::None) {
    refreshMode = RefreshMode::UpdatedFiles;
  }

  bool allowsShallowIndexing = false;
  for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
    if(sourceGroup->getStatus() == SOURCE_GROUP_STATUS_ENABLED && sourceGroup->allowsShallowIndexing()) {
      allowsShallowIndexing = true;
      break;
    }
  }

  const bool useShallowIndexing = allowsShallowIndexing && shallowIndexingRequested;

  if(m_hasGUI) {
    std::vector<RefreshMode> enabledModes = {RefreshMode::AllFiles};
    if(!needsFullRefresh) {
      enabledModes.insert(enabledModes.end(), {RefreshMode::UpdatedFiles, RefreshMode::UpdatedAndIncompleteFiles});
    }

    dialogView->startIndexingDialog(
        this,
        enabledModes,
        refreshMode,
        allowsShallowIndexing,
        useShallowIndexing,
        [this, dialogView](const RefreshInfo& info) { buildIndex(info, dialogView); },
        [this]() { m_refreshStage = RefreshStageType::NONE; });
  } else {
    RefreshInfo info = getRefreshInfo(refreshMode);
    info.shallow = useShallowIndexing;
    buildIndex(info, dialogView);
  }
}

RefreshInfo Project::getRefreshInfo(RefreshMode mode) const {
  switch(mode) {
  case RefreshMode::None:
    return {};

  case RefreshMode::UpdatedFiles:
    return RefreshInfoGenerator::getRefreshInfoForUpdatedFiles(m_sourceGroups, m_storage);

  case RefreshMode::UpdatedAndIncompleteFiles:
    return RefreshInfoGenerator::getRefreshInfoForIncompleteFiles(m_sourceGroups, m_storage);

  case RefreshMode::AllFiles:
  default:
    return RefreshInfoGenerator::getRefreshInfoForAllFiles(m_sourceGroups);
  }
}

void Project::buildIndex(RefreshInfo info, std::shared_ptr<DialogView> dialogView) {
  assert(dialogView);
  // Check the project status if it's indexing
  if(RefreshStageType::INDEXING == m_refreshStage) {
    MessageStatus(L"Cannot refresh project while indexing.", true, false).dispatch();
    return;
  }

  if(checkIfNothingToRefresh(info, dialogView)) {
    return;
  }

  if(checkIfFilesToClear(info, dialogView)) {
    return;
  }

  // Start
  MessageStatus(L"Preparing Indexing", false, true).dispatch();
  MessageErrorCountClear().dispatch();

  dialogView->showUnknownProgressDialog(L"Preparing Indexing", L"Setting up Indexers");
  MessageIndexingStatus(true, 0).dispatch();

  // Clear storage cache
  m_storageCache->clear();
  m_storageCache->setSubject(m_storage);

  const FilePath indexDbFilePath = m_settings->getDBFilePath();
  const FilePath tempIndexDbFilePath = m_settings->getTempDBFilePath();

  // store the indexed data into the temp db but keep the current state to allow browsing while indexing
  if(RefreshMode::AllFiles != info.mode) {
    std::ignore = FileSystem::copyFile(indexDbFilePath, tempIndexDbFilePath);
  }

  // Create new temp storage
  // TODO(SOUR-102): Create PersistentStorage using factory pattern
  const auto tempStorage = std::make_shared<PersistentStorage>(tempIndexDbFilePath, m_storage->getBookmarkDbFilePath());
  tempStorage->setup();
  // Store the setting at temp storage
  tempStorage->setProjectSettingsText(TextAccess::createFromFile(getProjectSettingsFilePath())->getText());
  tempStorage->updateVersion();

  size_t sourceFileCount{};
  Task::dispatch(TabId::app(), createIndexTasks(info, dialogView, tempStorage, sourceFileCount));

  m_refreshStage = RefreshStageType::INDEXING;
  MessageStatus(fmt::format(L"Starting Indexing: {} source files", sourceFileCount), false, true).dispatch();
  MessageIndexingStarted().dispatch();
}

void Project::swapToTempStorage(std::shared_ptr<DialogView> dialogView) {
  LOG_INFO("Switching to temporary indexing data");

  const FilePath indexDbFilePath = m_settings->getDBFilePath();
  const FilePath tempIndexDbFilePath = m_settings->getTempDBFilePath();
  const FilePath bookmarkDbFilePath = m_settings->getBookmarkDBFilePath();

  m_storage.reset();

  if(!swapToTempStorageFile(indexDbFilePath, tempIndexDbFilePath, dialogView)) {
    m_state = ProjectStateType::NOT_LOADED;
    return;
  }

  m_storage = std::make_shared<PersistentStorage>(indexDbFilePath, bookmarkDbFilePath);
  m_storage->setup();

  // std::shared_ptr<DialogView> dialogView =
  // Application::getInstance()->getDialogView(DialogView::UseCase::INDEXING);
  // dialogView->showUnknownProgressDialog(L"Finish Indexing", L"Building caches");
  m_storage->buildCaches();
  // dialogView->hideUnknownProgressDialog();

  m_storageCache->setSubject(m_storage);
  m_state = ProjectStateType::LOADED;
}

bool Project::swapToTempStorageFile(const FilePath& indexDbFilePath,
                                    const FilePath& tempIndexDbFilePath,
                                    std::shared_ptr<DialogView> dialogView) {
  try {
    FileSystem::remove(indexDbFilePath);
    FileSystem::rename(tempIndexDbFilePath, indexDbFilePath);
  } catch(std::exception& /*e*/) {
    if(m_hasGUI) {
      dialogView->confirm(
          L"<p>The old index database file of this project seems to be used by a different "
          L"process and cannot "
          L"be updated.</p><p>Please close all processes that are using this database and "
          L"re-load this project to "
          L"apply or discard the changes pending from the current indexer run.</p>");
    }
    return false;
  }
  return true;
}

void Project::discardTempStorage() {
  const FilePath tempIndexDbPath = m_settings->getTempDBFilePath();
  if(tempIndexDbPath.exists()) {
    LOG_INFO("Discarding temporary indexing data");
    FileSystem::remove(tempIndexDbPath);
  }
}

bool Project::hasCxxSourceGroup() const {
#  if BUILD_CXX_LANGUAGE_PACKAGE
  for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
    if(sourceGroup->getStatus() == SOURCE_GROUP_STATUS_ENABLED) {
      if(sourceGroup->getLanguage() == LANGUAGE_C || sourceGroup->getLanguage() == LANGUAGE_CPP) {
        return true;
      }
    }
  }
#  endif    // BUILD_CXX_LANGUAGE_PACKAGE
  return false;
}

std::shared_ptr<TaskGroupSequence> Project::createIndexTasks(RefreshInfo info,
                                                             std::shared_ptr<DialogView> dialogView,
                                                             std::shared_ptr<PersistentStorage> tempStorage,
                                                             size_t& sourceFileCount) {
  auto taskSequential = std::make_shared<TaskGroupSequence>();

  // Add task to clear the storage in case of refresh all
  if(RefreshMode::AllFiles != info.mode && (!info.filesToClear.empty() || !info.nonIndexedFilesToClear.empty())) {
    taskSequential->addTask(
        // TODO(Hussein): Create Tasks using factory pattern
        std::make_shared<TaskCleanStorage>(tempStorage,
                                           dialogView,
                                           utility::toVector(utility::concat(info.filesToClear, info.nonIndexedFilesToClear)),
                                           info.mode == RefreshMode::UpdatedAndIncompleteFiles));
  }

  // TODO(Hussein): Create Tasks using factory pattern
  auto indexerCommandProvider = std::make_unique<CombinedIndexerCommandProvider>();
  auto customIndexerCommandProvider = std::make_unique<CombinedIndexerCommandProvider>();
  for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
    if(SOURCE_GROUP_STATUS_ENABLED == sourceGroup->getStatus()) {
      if(SOURCE_GROUP_CUSTOM_COMMAND == sourceGroup->getType()) {
        customIndexerCommandProvider->addProvider(sourceGroup->getIndexerCommandProvider(info));
      } else {
        indexerCommandProvider->addProvider(sourceGroup->getIndexerCommandProvider(info));
      }
    }
  }

  sourceFileCount = indexerCommandProvider->size() + customIndexerCommandProvider->size();

  // TODO(Hussein): Create Tasks using factory pattern
  taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("shallow_indexing", info.shallow));
  taskSequential->addTask(std::make_shared<TaskSetValue<int>>("source_file_count", static_cast<int>(sourceFileCount)));
  taskSequential->addTask(std::make_shared<TaskSetValue<int>>("indexed_source_file_count", 0));
  taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("interrupted_indexing", false));
  taskSequential->addTask(std::make_shared<TaskSetValue<float>>("index_time", 0.0F));

  const int indexerThreadCount = getIndexerThreadCount();

  if(!indexerCommandProvider->empty()) {
    const int adjustedIndexerThreadCount = std::min<int>(indexerThreadCount, static_cast<int>(indexerCommandProvider->size()));

    // TODO(Hussein): Create Tasks using factory pattern
    auto storageProvider = std::make_shared<StorageProvider>();
    // add tasks for setting some variables on the blackboard that are used during indexing
    taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("indexer_threads_started", false));
    taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("indexer_threads_stopped", false));
    taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("indexer_command_queue_started", false));
    taskSequential->addTask(std::make_shared<TaskSetValue<bool>>("indexer_command_queue_stopped", false));

    // TODO(Hussein): Create Tasks using factory pattern
    auto preIndexTasks = std::make_shared<TaskGroupSequence>();
    taskSequential->addTask(preIndexTasks);
    for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
      if(sourceGroup->getStatus() == SOURCE_GROUP_STATUS_ENABLED) {
        preIndexTasks->addTask(sourceGroup->getPreIndexTask(storageProvider, dialogView));
      }
    }

    // TODO(Hussein): Create Tasks using factory pattern
    auto taskParserWrapper = std::make_shared<TaskParseWrapper>(tempStorage, dialogView);
    taskSequential->addTask(taskParserWrapper);

    // TODO(Hussein): Create Tasks using factory pattern
    auto taskParallelIndexing = std::make_shared<TaskGroupParallel>();
    taskParserWrapper->setTask(taskParallelIndexing);

    // add task for refilling the indexer command queue
    // TODO(Hussein): Create Tasks using factory pattern
    taskParallelIndexing->addTask(std::make_shared<TaskFillIndexerCommandsQueue>(m_appUUID, std::move(indexerCommandProvider), 20));

    // add task for indexing
    const bool multiProcess = IApplicationSettings::getInstanceRaw()->getMultiProcessIndexingEnabled() && hasCxxSourceGroup();
    taskParallelIndexing->addChildTasks(std::make_shared<TaskGroupSequence>()->addChildTasks(
        // block until there are indexer commands to process
        // TODO(Hussein): Create Tasks using factory pattern
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 25)
            ->addChildTask(std::make_shared<TaskReturnSuccessIf<bool>>(
                "indexer_command_queue_started", TaskReturnSuccessIf<bool>::CONDITION_EQUALS, false)),
        std::make_shared<TaskBuildIndex>(adjustedIndexerThreadCount, storageProvider, dialogView, m_appUUID, multiProcess)));

    // add task for merging the intermediate storages
    taskParallelIndexing->addTask(std::make_shared<TaskGroupSequence>()->addChildTasks(
        // block until there are indexers running
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 25)
            ->addChildTask(std::make_shared<TaskReturnSuccessIf<bool>>(
                "indexer_threads_started", TaskReturnSuccessIf<bool>::CONDITION_EQUALS, false)),
        // merge until all indexers stopped and nothing left to merge
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 250)
            ->addChildTask(std::make_shared<TaskGroupSelector>()->addChildTasks(
                std::make_shared<TaskMergeStorages>(storageProvider),
                std::make_shared<TaskReturnSuccessIf<bool>>(
                    "indexer_threads_stopped", TaskReturnSuccessIf<bool>::CONDITION_EQUALS, false)))));

    // add task for injecting the intermediate storages into the persistent storage
    taskParallelIndexing->addTask(std::make_shared<TaskGroupSequence>()->addChildTasks(
        // block until there are indexers running
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 25)
            ->addChildTask(std::make_shared<TaskReturnSuccessIf<bool>>(
                "indexer_threads_started", TaskReturnSuccessIf<bool>::CONDITION_EQUALS, false)),
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 25)
            ->addChildTask(std::make_shared<TaskGroupSelector>()->addChildTasks(
                std::make_shared<TaskInjectStorage>(storageProvider, tempStorage),
                // continuing when indexers still running, even if there are no storages right now.
                std::make_shared<TaskReturnSuccessIf<bool>>(
                    "indexer_threads_stopped", TaskReturnSuccessIf<bool>::CONDITION_EQUALS, false)))));

    // add task that notifies the user of what's going on
    taskSequential->addTask(    // we don't need to hide this dialog again, because it's
                                // overridden by other dialogs later on.
        std::make_shared<TaskLambda>(
            [dialogView]() { dialogView->showUnknownProgressDialog(L"Finish Indexing", L"Saving\nRemaining Data"); }));

    // add task that injects the remaining intermediate storages into the persistent storage
    taskSequential->addTask(
        std::make_shared<TaskDecoratorRepeat>(TaskDecoratorRepeat::CONDITION_WHILE_SUCCESS, Task::STATE_SUCCESS, 25)
            ->addChildTask(std::make_shared<TaskInjectStorage>(storageProvider, tempStorage)));
  } else {
    dialogView->hideUnknownProgressDialog();
  }

  if(!customIndexerCommandProvider->empty()) {
    const int adjustedIndexerThreadCount = std::min<int>(
        indexerThreadCount, static_cast<int>(customIndexerCommandProvider->size()));

    taskSequential->addTask(std::make_shared<TaskExecuteCustomCommands>(std::move(customIndexerCommandProvider),
                                                                        tempStorage,
                                                                        dialogView,
                                                                        adjustedIndexerThreadCount,
                                                                        getProjectSettingsFilePath().getParentDirectory()));
  }

  // TODO(Hussein): Create Tasks using factory pattern
  taskSequential->addTask(std::make_shared<TaskFinishParsing>(tempStorage, dialogView));

  taskSequential->addTask(std::make_shared<TaskGroupSelector>()->addChildTasks(
      std::make_shared<TaskGroupSequence>()->addChildTasks(
          std::make_shared<TaskFindKeyOnBlackboard>("keep_database"), std::make_shared<TaskLambda>([dialogView, this]() {
            Task::dispatch(TabId::app(), std::make_shared<TaskLambda>([dialogView, this]() { swapToTempStorage(dialogView); }));
          })),
      std::make_shared<TaskGroupSequence>()->addChildTasks(
          std::make_shared<TaskFindKeyOnBlackboard>("discard_database"), std::make_shared<TaskLambda>([this]() {
            Task::dispatch(TabId::app(), std::make_shared<TaskLambda>([this]() { discardTempStorage(); }));
          }))));

  // TODO(Hussein): Create Tasks using factory pattern
  taskSequential->addTask(std::make_shared<TaskLambda>([dialogView, this]() {
    m_refreshStage = RefreshStageType::NONE;
    MessageIndexingFinished().dispatch();
  }));

  taskSequential->addTask(std::make_shared<TaskGroupSelector>()->addChildTasks(
      std::make_shared<TaskGroupSequence>()->addChildTasks(
          std::make_shared<TaskFindKeyOnBlackboard>("refresh_database"), std::make_shared<TaskLambda>([dialogView]() {
            Task::dispatch(TabId::app(), std::make_shared<TaskLambda>([dialogView]() {
                             MessageIndexingShowDialog().dispatch();
                             MessageRefresh().refreshAll().dispatch();
                           }));
          })),
      std::make_shared<TaskGroupSequence>()->addChildTasks(
          std::make_shared<TaskLambda>([]() { Task::dispatch(TabId::app(), std::make_shared<TaskLambda>([]() {})); }))));

  taskSequential->setIsBackgroundTask(true);
  return taskSequential;
}

bool Project::checkIfNothingToRefresh(const RefreshInfo& info, std::shared_ptr<DialogView> dialogView) {
  std::wstring message;
  if(info.mode != RefreshMode::AllFiles && info.filesToClear.empty() && info.filesToIndex.empty()) {
    message = L"Nothing to refresh, all files are up-to-date.";
  } else if(m_sourceGroups.empty()) {
    message = L"Nothing to refresh, no Source Groups loaded.";
  }

  if(!message.empty()) {
    if(m_hasGUI) {
      dialogView->clearDialogs();
    } else {
      MessageIndexingFinished().dispatch();
    }

    MessageStatus(message).dispatch();
    m_refreshStage = RefreshStageType::NONE;
    return true;
  }
  return false;
}

bool Project::checkIfFilesToClear(RefreshInfo& info, std::shared_ptr<DialogView> dialogView) {
  if(RefreshMode::AllFiles != info.mode && (!info.filesToClear.empty() || !info.nonIndexedFilesToClear.empty())) {
    for(const std::shared_ptr<SourceGroup>& sourceGroup : m_sourceGroups) {
      if(SOURCE_GROUP_STATUS_ENABLED == sourceGroup->getStatus() && !sourceGroup->allowsPartialClearing()) {
        if(ranges::any_of(info.filesToClear,
                          [&sourceGroup](const auto& sourcePath) { return sourceGroup->containsSourceFilePath(sourcePath); }) ||
           ranges::any_of(info.nonIndexedFilesToClear,
                          [&sourceGroup](const auto& sourcePath) { return sourceGroup->containsSourceFilePath(sourcePath); })) {
          if(m_hasGUI &&
             dialogView->confirm(fmt::format(L"<p>This project contains a source group of type \"{}\" that cannot be partially "
                                             L"cleared. Do you want to re-index the whole project instead?</p>",
                                             utility::decodeFromUtf8(sourceGroupTypeToString(sourceGroup->getType()))),
                                 {L"Full Re-Index", L"Cancel"}) == 1) {
            MessageStatus(L"Cannot partially clear project. Indexing aborted.").dispatch();
            m_refreshStage = RefreshStageType::NONE;
            dialogView->clearDialogs();
            return true;
          }
          const bool shallow = info.shallow;
          info = getRefreshInfo(RefreshMode::AllFiles);
          info.shallow = shallow;
        }

        break;
      }
    }
  }
  return false;
}
#endif
