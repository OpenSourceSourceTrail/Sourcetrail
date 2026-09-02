#include "project/Project.h"

#include <algorithm>
#include <cassert>
#include <tuple>
#include <utility>

#include <fmt/format.h>

#include "app/IndexerPluginRegistry.h"
#include "component/view/DialogView.h"
#include "data/storage/PersistentStorage.h"
#include "data/storage/StorageCache.h"
#include "error/messages/MessageErrorCountClear.h"
#include "FilePath.h"
#include "FileSystem.h"
#include "indexing/domain/IndexingPhaseStats.h"
#include "indexing/messages/MessageIndexingFinished.h"
#include "indexing/messages/MessageIndexingStarted.h"
#include "indexing/messages/MessageIndexingStatus.h"
#include "project/DefaultTaskFactory.h"
#include "project/IndexTaskBuilder.h"
#include "project/ITaskFactory.h"
#include "project/RefreshInfoGenerator.h"
#include "project/SourceGroup.h"
#include "project/SourceGroupFactory.h"
#include "refresh/messages/MessageRefresh.h"
#include "settings/IApplicationSettings.hpp"
#include "settings/ProjectSettings.h"
#include "settings/source_group/SourceGroupSettings.h"
#include "settings/source_group/SourceGroupStatusType.h"
#include "status/messages/MessageStatus.h"
#include "TabId.h"
#include "Task.h"
#include "TaskGroupSequence.h"
#include "TextAccess.h"
#include "utility.h"
#include "utilityApp.h"
#include "utilityString.h"

Project::Project(std::shared_ptr<ProjectSettings> settings,
                 std::shared_ptr<StorageCache> storageCache,
                 std::string appUUID,
                 bool hasGUI,
                 std::shared_ptr<ITaskFactory> taskFactory) noexcept
    : m_settings(std::move(settings)), m_storageCache(std::move(storageCache)), m_appUUID(std::move(appUUID)), m_hasGUI(hasGUI) {
  IndexTaskBuilder::Callbacks callbacks;
  callbacks.onKeepDatabase = [this](std::shared_ptr<DialogView> dialogView) { swapToTempStorage(std::move(dialogView)); };
  callbacks.onDiscardDatabase = [this]() { discardTempStorage(); };
  callbacks.onIndexingFinished = [this]() {
    m_refreshStage = RefreshStageType::NONE;
    MessageIndexingFinished().dispatch();
  };

  m_indexTaskBuilder = std::make_unique<IndexTaskBuilder>(
      taskFactory ? std::move(taskFactory) : std::make_shared<DefaultTaskFactory>(), m_appUUID, std::move(callbacks));
}

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

bool Project::isReindexable() const {
  const IndexerPluginRegistry::Ptr pluginRegistry = IndexerPluginRegistry::getInstance();
  const std::shared_ptr<SourceGroupFactory> sourceGroupFactory = SourceGroupFactory::getInstance();
  for(const std::shared_ptr<SourceGroupSettings>& sourceGroupSettings : m_settings->getAllSourceGroupSettings()) {
    const SourceGroupType type = sourceGroupSettings->getType();
    // A type is reindexable if it is covered by a discovered plugin, or already handled by a
    // compiled-in source group module (keeps the default in-tree build working without plugins).
    if(!pluginRegistry->supportsSourceGroupType(type) && !sourceGroupFactory->supports(type)) {
      return false;
    }
  }
  return true;
}

bool Project::settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const {
  return m_settings->equalsExceptNameAndLocation(otherSettings);
}

void Project::setStateOutdated() {
  if(ProjectStateType::LOADED == m_state) {
    m_state = ProjectStateType::OUTDATED;
  }
}

void Project::load(const std::shared_ptr<DialogView>& dialogView) {
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

  m_sourceGroups = SourceGroupFactory::getInstance()->createSourceGroups(m_settings->getAllSourceGroupSettings());

  if(canLoad) {
    m_storage->setMode(SqliteIndexStorage::STORAGE_MODE_READ);
    m_storage->buildCaches();
    m_storageCache->setSubject(m_storage);

    if(m_hasGUI) {
      MessageIndexingFinished().dispatch();
    }
    MessageStatus(L"Finished Loading", false, false).dispatch();
  } else {
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
  }

  if(m_state != ProjectStateType::LOADED && m_hasGUI) {
    MessageRefresh().dispatch();
  }
}

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

  if(!question.empty() && m_hasGUI) {
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

  // Everything the pipeline measured, once the last cache is built: this is the only point where
  // the whole run -- merge, inject, caches, VACUUM -- has finished.
  indexing_stats::print();

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

std::shared_ptr<TaskGroupSequence> Project::createIndexTasks(RefreshInfo info,
                                                             std::shared_ptr<DialogView> dialogView,
                                                             std::shared_ptr<PersistentStorage> tempStorage,
                                                             size_t& sourceFileCount) {
  return m_indexTaskBuilder->createIndexTasks(std::move(info),
                                              std::move(dialogView),
                                              std::move(tempStorage),
                                              m_sourceGroups,
                                              getProjectSettingsFilePath().getParentDirectory(),
                                              sourceFileCount);
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
        if(std::ranges::any_of(info.filesToClear,
                               [&sourceGroup](const auto& sourcePath) { return sourceGroup->containsSourceFilePath(sourcePath); }) ||
           std::ranges::any_of(info.nonIndexedFilesToClear, [&sourceGroup](const auto& sourcePath) {
             return sourceGroup->containsSourceFilePath(sourcePath);
           })) {
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
