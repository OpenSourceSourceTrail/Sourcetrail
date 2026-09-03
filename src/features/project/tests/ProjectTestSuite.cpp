#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FilePath.h"
#include "MessageListener.h"
#include "status/messages/MessageStatus.h"
#define private public
#include "project/data/Project.h"
#undef private
#include "data/storage/PersistentStorage.h"
#include "indexing/logic/IndexerCommandProvider.h"
#include "MockedApplicationSetting.hpp"
#include "mocks/MockedDialogView.hpp"
#include "mocks/MockedMessageQueue.hpp"
#include "mocks/MockedProjectSettings.hpp"
#include "mocks/MockedStorageCache.hpp"
#include "project/data/DefaultTaskFactory.h"
#include "project/tests/MockedSourceGroup.hpp"
#include "settings/IApplicationSettings.hpp"
#include "settings/source_group/SourceGroupSettings.h"
#include "settings/source_group/type/SourceGroupSettingsCppEmpty.h"
#include "TabId.h"
#include "TaskDispatchRegistry.h"

using namespace std::chrono_literals;
using namespace testing;

constexpr auto WaitingDuration = 100ms;

// TODO(Hussein): Move to mocks folder
struct MockedMessageStatus : MessageListener<MessageStatus> {
  MOCK_METHOD(void, handleMessage, (MessageStatus*), (override));
};

struct MockedPersistentStorage : PersistentStorage {
  MockedPersistentStorage() : PersistentStorage{FilePath{}, FilePath{}} {}

  using ReturnAddNode = std::pair<Id, bool>;
  MOCK_METHOD(ReturnAddNode, addNode, (const StorageNodeData&), (override));
  MOCK_METHOD(std::vector<Id>, addNodes, (const std::vector<StorageNode>&), (override));

  MOCK_METHOD(void, addSymbol, (const StorageSymbol&), (override));
  MOCK_METHOD(void, addSymbols, (const std::vector<StorageSymbol>&), (override));

  MOCK_METHOD(void, addFile, (const StorageFile&), (override));

  MOCK_METHOD(Id, addEdge, (const StorageEdgeData& data), (override));
  MOCK_METHOD(std::vector<Id>, addEdges, (const std::vector<StorageEdge>& edges), (override));

  MOCK_METHOD(Id, addLocalSymbol, (const StorageLocalSymbolData& data), (override));
  MOCK_METHOD(std::vector<Id>, addLocalSymbols, (const std::set<StorageLocalSymbol>& symbols), (override));

  MOCK_METHOD(Id, addSourceLocation, (const StorageSourceLocationData& data), (override));
  MOCK_METHOD(std::vector<Id>, addSourceLocations, (const std::vector<StorageSourceLocation>& locations), (override));

  MOCK_METHOD(void, addOccurrence, (const StorageOccurrence& data), (override));
  MOCK_METHOD(void, addOccurrences, (const std::vector<StorageOccurrence>& occurrences), (override));

  MOCK_METHOD(void, addComponentAccess, (const StorageComponentAccess& componentAccess), (override));
  MOCK_METHOD(void, addComponentAccesses, (const std::vector<StorageComponentAccess>& componentAccesses), (override));

  MOCK_METHOD(void, addElementComponent, (const StorageElementComponent& component), (override));
  MOCK_METHOD(void, addElementComponents, (const std::vector<StorageElementComponent>& components), (override));

  MOCK_METHOD(Id, addError, (const StorageErrorData& data), (override));

  MOCK_METHOD(const std::vector<StorageNode>&, getStorageNodes, (), (const, override));
  MOCK_METHOD(const std::vector<StorageFile>&, getStorageFiles, (), (const, override));
  MOCK_METHOD(const std::vector<StorageSymbol>&, getStorageSymbols, (), (const, override));
  MOCK_METHOD(const std::vector<StorageEdge>&, getStorageEdges, (), (const, override));
  MOCK_METHOD(const std::set<StorageLocalSymbol>&, getStorageLocalSymbols, (), (const, override));
  MOCK_METHOD(const std::set<StorageSourceLocation>&, getStorageSourceLocations, (), (const, override));
  MOCK_METHOD(const std::set<StorageOccurrence>&, getStorageOccurrences, (), (const, override));
  MOCK_METHOD(const std::set<StorageComponentAccess>&, getComponentAccesses, (), (const, override));
  MOCK_METHOD(const std::set<StorageElementComponent>&, getElementComponents, (), (const, override));
  MOCK_METHOD(const std::vector<StorageError>&, getErrors, (), (const, override));

  MOCK_METHOD(void, startInjection, (), (override));
  MOCK_METHOD(void, finishInjection, (), (override));
};

struct MockedIndexerCommandProvider : IndexerCommandProvider {
  MOCK_METHOD(std::vector<FilePath>, getAllSourceFilePaths, (), (const, override));
  MOCK_METHOD(std::shared_ptr<IndexerCommand>, consumeCommand, (), (override));
  MOCK_METHOD(std::shared_ptr<IndexerCommand>, consumeCommandForSourceFilePath, (const FilePath& filePath), (override));
  MOCK_METHOD(std::vector<std::shared_ptr<IndexerCommand>>, consumeAllCommands, (), (override));
  MOCK_METHOD(void, clear, (), (override));
  MOCK_METHOD(size_t, size, (), (const, override));
};

// Project::buildIndex() ends by dispatching the index task graph to the global TabId::app()
// queue, which runs it on a background thread. A unit test has no way to wait for that graph,
// so whether it runs at all -- and how far it gets before the fixture tears the mocks, the
// message queue and the dispatch queue down under it -- is pure timing: it stayed dormant on
// fast machines and crashed the process on slower CI runners. Feed Project a task factory whose
// sequences discard their children instead: the graph is still built and dispatched, exercising
// buildIndex() end to end, but the dispatched task is inert and touches nothing the fixture owns.
struct InertTaskGroupSequence final : TaskGroupSequence {
  void addTask(std::shared_ptr<Task> /*task*/) override {}
};

struct InertTaskFactory final : ITaskFactory {
  std::shared_ptr<TaskGroupSequence> createSequence() override {
    return std::make_shared<InertTaskGroupSequence>();
  }

  std::shared_ptr<TaskGroupSelector> createSelector() override {
    return mDefault.createSelector();
  }

  std::shared_ptr<TaskflowGroupParallel> createParallel() override {
    return mDefault.createParallel();
  }

  std::shared_ptr<TaskFindKeyOnBlackboard> createFindKeyOnBlackboard(const std::string& valueName) override {
    return mDefault.createFindKeyOnBlackboard(valueName);
  }

  std::shared_ptr<TaskLambda> createLambda(std::function<void()> func) override {
    return mDefault.createLambda(std::move(func));
  }

  std::shared_ptr<TaskDecoratorRepeat> createRepeat(TaskDecoratorRepeat::ConditionType condition,
                                                    Task::TaskState exitState,
                                                    size_t delayMS) override {
    return mDefault.createRepeat(condition, exitState, delayMS);
  }

  std::shared_ptr<TaskCleanStorage> createCleanStorage(std::weak_ptr<PersistentStorage> storage,
                                                       std::shared_ptr<DialogView> dialogView,
                                                       const std::vector<FilePath>& filePaths,
                                                       bool clearAllErrors) override {
    return mDefault.createCleanStorage(std::move(storage), std::move(dialogView), filePaths, clearAllErrors);
  }

  std::shared_ptr<TaskParseWrapper> createParseWrapper(std::weak_ptr<PersistentStorage> storage,
                                                       std::shared_ptr<DialogView> dialogView) override {
    return mDefault.createParseWrapper(std::move(storage), std::move(dialogView));
  }

  std::shared_ptr<TaskFillIndexerCommandsQueue> createFillIndexerCommandsQueue(
      std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
      std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
      size_t maximumQueueSize) override {
    return mDefault.createFillIndexerCommandsQueue(
        std::move(indexerWorkerService), std::move(indexerCommandProvider), maximumQueueSize);
  }

  std::shared_ptr<TaskBuildIndex> createBuildIndex(size_t processCount,
                                                   std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                                                   std::shared_ptr<StorageProvider> storageProvider,
                                                   std::shared_ptr<DialogView> dialogView,
                                                   std::string appUUID,
                                                   IndexerCommandType commandType) override {
    return mDefault.createBuildIndex(processCount,
                                     std::move(indexerWorkerService),
                                     std::move(storageProvider),
                                     std::move(dialogView),
                                     std::move(appUUID),
                                     commandType);
  }

  std::shared_ptr<TaskMergeStorages> createMergeStorages(std::shared_ptr<StorageProvider> storageProvider) override {
    return mDefault.createMergeStorages(std::move(storageProvider));
  }

  std::shared_ptr<TaskInjectStorage> createInjectStorage(std::shared_ptr<StorageProvider> storageProvider,
                                                         std::weak_ptr<Storage> target) override {
    return mDefault.createInjectStorage(std::move(storageProvider), std::move(target));
  }

  std::shared_ptr<TaskExecuteCustomCommands> createExecuteCustomCommands(std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                                                                         std::shared_ptr<PersistentStorage> storage,
                                                                         std::shared_ptr<DialogView> dialogView,
                                                                         size_t indexerThreadCount,
                                                                         FilePath projectDirectory) override {
    return mDefault.createExecuteCustomCommands(std::move(indexerCommandProvider),
                                                std::move(storage),
                                                std::move(dialogView),
                                                indexerThreadCount,
                                                std::move(projectDirectory));
  }

  std::shared_ptr<TaskFinishParsing> createFinishParsing(std::shared_ptr<PersistentStorage> storage,
                                                         std::shared_ptr<DialogView> dialogView) override {
    return mDefault.createFinishParsing(std::move(storage), std::move(dialogView));
  }

private:
  DefaultTaskFactory mDefault;
};

struct ProjectFix : Test {
  void SetUp() override {
    mMockedMessageQueue = std::make_shared<StrictMock<MockedMessageQueue>>();
    IMessageQueue::setInstance(mMockedMessageQueue);

    mStatus = std::unique_ptr<MockedMessageStatus>();

    mSettings = std::make_shared<StrictMock<MockedProjectSettings>>();
    mStorageCache = std::make_shared<StrictMock<MockedStorageCache>>();
    mProject = std::make_unique<Project>(mSettings, mStorageCache, "", true, std::make_shared<InertTaskFactory>());

    mDialogView = std::make_shared<StrictMock<MockedDialogView>>();

    IApplicationSettings::setInstance(std::make_shared<MockedApplicationSettings>());
  }

  void TearDown() override {
    // Join whatever buildIndex() may have dispatched to TabId::app() before
    // the mocks below get destroyed out from under a still-running background thread.
    TaskDispatchRegistry::getInstance().destroyQueue(TabId::app());

    IApplicationSettings::setInstance(nullptr);
    IMessageQueue::setInstance(nullptr);
    mMockedMessageQueue.reset();
  }

  // refresh() reloads the settings before it opens any dialog -- reload() is non-virtual and lands
  // on load(). Not what these tests are about, so it is allowed rather than asserted on.
  void MockRefreshPreamble() {
    EXPECT_CALL(*mSettings, load(_, _)).WillRepeatedly(Return(true));
  }

  std::shared_ptr<StrictMock<MockedMessageQueue>> mMockedMessageQueue;
  std::shared_ptr<StrictMock<MockedProjectSettings>> mSettings;
  std::shared_ptr<StrictMock<MockedStorageCache>> mStorageCache;
  std::unique_ptr<Project> mProject;
  std::shared_ptr<StrictMock<MockedDialogView>> mDialogView;
  std::unique_ptr<MockedMessageStatus> mStatus;
};

TEST_F(ProjectFix, getProjectSettingsFilePath) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Get ProjectSettings filePath
  const auto filePath = mProject->getProjectSettingsFilePath();

  // Then: The file is empty
  EXPECT_TRUE(filePath.empty());
}

TEST_F(ProjectFix, getDescription) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Get Description
  const auto description = mProject->getDescription();

  // Then: Description is empty
  EXPECT_TRUE(description.empty());
}

TEST_F(ProjectFix, isLoadedDefaultBehave) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Checking the project is loaded
  const auto isLoaded = mProject->isLoaded();

  // Then: The project isn't loaded
  EXPECT_FALSE(isLoaded);
}

TEST_F(ProjectFix, isLoaded) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());
  // And: Project is Loaded
  mProject->m_state = Project::ProjectStateType::LOADED;

  // When: Checking the project is loaded
  const auto isLoaded = mProject->isLoaded();

  // Then: The project is loaded
  EXPECT_TRUE(isLoaded);
}

TEST_F(ProjectFix, isIndexingDefaultBehave) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Checking the project is indexing
  const auto isIndexing = mProject->isIndexing();

  // Then: The project isn't indexing
  EXPECT_FALSE(isIndexing);
}

TEST_F(ProjectFix, isIndexing) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());
  // And: Project is indexing
  mProject->m_refreshStage = Project::RefreshStageType::INDEXING;

  // When: Checking the project is indexing
  const auto isIndexing = mProject->isIndexing();

  // Then: The project is indexing
  EXPECT_TRUE(isIndexing);
}

TEST_F(ProjectFix, settingsEqualExceptNameAndLocation) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Checking the project is indexing
  const auto flag = mProject->settingsEqualExceptNameAndLocation(MockedProjectSettings{});

  // Then: The project is indexing
  EXPECT_TRUE(flag);
}

TEST_F(ProjectFix, setStateOutdated) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());

  // When: Checking the project is loaded
  mProject->setStateOutdated();

  // Then: The project is loaded
  EXPECT_NE(Project::ProjectStateType::OUTDATED, mProject->m_state);
}

TEST_F(ProjectFix, setStateOutdatedOutdated) {
  // Given: The project exists
  ASSERT_THAT(mProject, testing::NotNull());
  // And: The project state is lodaded
  mProject->m_state = Project::ProjectStateType::LOADED;

  // When: Checking the project is loaded
  mProject->setStateOutdated();

  // Then: The project is loaded
  EXPECT_EQ(Project::ProjectStateType::OUTDATED, mProject->m_state);
}

TEST_F(ProjectFix, loadWhileIndexing) {
  // Given: The project is indexing
  mProject->m_refreshStage = Project::RefreshStageType::INDEXING;
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).WillOnce(testing::Return());

  // When: Load a project
  mProject->load(mDialogView);

  // Then: Load returns
}

TEST_F(ProjectFix, loadFailedToReloadSettings) {
  // Given: The project status is none
  // ASSERT_EQ(Project::RefreshStageType::NONE, mProject->m_refreshStage);
  EXPECT_CALL(*mStorageCache, setUseErrorCache(_)).WillOnce(Return());
  EXPECT_CALL(*mSettings, load(_, _)).WillOnce(Return(false));

  // When: Load a project
  mProject->load(mDialogView);

  // Then: Load returns
}

// TODO(Hussein): The test isn't complete
TEST_F(ProjectFix, DISABLED_loadGoodCase) {
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).Times(2);
  EXPECT_CALL(*mSettings, load(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mStorageCache, setUseErrorCache(_)).WillOnce(Return());

  mProject->load(mDialogView);

  std::this_thread::sleep_for(WaitingDuration);

  EXPECT_TRUE(mProject->isLoaded());
  EXPECT_FALSE(mProject->isIndexing());
}

// TODO(Hussein): The test isn't complete
TEST_F(ProjectFix, DISABLED_loadFailed) {
  EXPECT_CALL(*mSettings, load(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mStorageCache, setUseErrorCache(_)).WillOnce(Return());

  // mProject->m_refreshStage = Project::RefreshStageType::NONE;
  mProject->load(mDialogView);

  EXPECT_TRUE(mProject->isLoaded());
  EXPECT_FALSE(mProject->isIndexing());
}

/*
 * Characterization tests for Project::refresh().
 *
 * Written before startIndexingDialog's signature changes, so the change can be shown not to alter
 * behavior. They pin what refresh() does today.
 */
TEST_F(ProjectFix, refreshIsANoOpWhileAlreadyRefreshing) {
  mProject->m_refreshStage = Project::RefreshStageType::INDEXING;

  // The StrictMock dialog view asserts that no dialog is opened.
  mProject->refresh(mDialogView, RefreshMode::AllFiles, false);

  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::INDEXING);
}

TEST_F(ProjectFix, refreshIsANoOpWhenNothingIsLoaded) {
  mProject->m_state = Project::ProjectStateType::NOT_LOADED;

  mProject->refresh(mDialogView, RefreshMode::AllFiles, false);

  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::NONE);
}

TEST_F(ProjectFix, refreshOnALoadedProjectOffersAllThreeModes) {
  mProject->m_state = Project::ProjectStateType::LOADED;
  MockRefreshPreamble();

  std::vector<RefreshMode> offeredModes;
  EXPECT_CALL(*mDialogView, startIndexingDialog(_, _, RefreshMode::AllFiles, false, false, _, _))
      .WillOnce([&offeredModes](auto&&, const std::vector<RefreshMode>& modes, auto&&, auto&&, auto&&, auto&&, auto&&) {
        offeredModes = modes;
      });

  mProject->refresh(mDialogView, RefreshMode::AllFiles, false);

  EXPECT_THAT(offeredModes, ElementsAre(RefreshMode::AllFiles, RefreshMode::UpdatedFiles, RefreshMode::UpdatedAndIncompleteFiles));
}

TEST_F(ProjectFix, refreshOnAnEmptyProjectOffersOnlyAFullReindex) {
  mProject->m_state = Project::ProjectStateType::EMPTY;
  MockRefreshPreamble();

  std::vector<RefreshMode> offeredModes;
  EXPECT_CALL(*mDialogView, startIndexingDialog(_, _, RefreshMode::AllFiles, false, false, _, _))
      .WillOnce([&offeredModes](auto&&, const std::vector<RefreshMode>& modes, auto&&, auto&&, auto&&, auto&&, auto&&) {
        offeredModes = modes;
      });

  mProject->refresh(mDialogView, RefreshMode::AllFiles, false);

  EXPECT_THAT(offeredModes, ElementsAre(RefreshMode::AllFiles));
}

TEST_F(ProjectFix, buildIndex_whileIndexing) {
  // Given: The project is indexing.
  mProject->m_refreshStage = Project::RefreshStageType::INDEXING;
  // And: Refresh all files flags.
  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, RefreshMode::AllFiles};
  // And: A message is called.
  EXPECT_CALL(*mMockedMessageQueue, pushMessage);

  // When: Call the build.
  mProject->buildIndex(info, mDialogView);

  // Then: The status keep the same.
  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::INDEXING);
}

TEST_F(ProjectFix, buildIndex_emptySourceGroups) {
  // Given: A message is called.
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).WillOnce(Return());
  // And: The clearDialogs is called.
  EXPECT_CALL(*mDialogView, clearDialogs).WillOnce(Return());
  // And: Refresh all files flags and empty source groups
  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, RefreshMode::AllFiles};

  // When: Call the build.
  mProject->buildIndex(info, mDialogView);

  // Then: The status keep the same.
  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::NONE);
}

TEST_F(ProjectFix, buildIndex_statusIsNoneAndNoFilesToIndex) {
  // Given: A message is called.
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).WillOnce(Return());
  // And: The clearDialogs is called.
  EXPECT_CALL(*mDialogView, clearDialogs).WillOnce(Return());
  // And: Refresh all files flags and empty source groups
  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, RefreshMode::None};

  // When: Call the build.
  mProject->buildIndex(info, mDialogView);

  // Then: The status keep the same.
  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::NONE);
}

#ifdef BUILD_CXX_LANGUAGE_PACKAGE
TEST_F(ProjectFix, buildIndex_statusIsNoneAndOneFileToClear) {
  // Given: Refresh none flags and one file to clear.
  const RefreshInfo info{{}, {FilePath{"1.cpp"}}, {}, RefreshMode::None};
  // And: SourceGroup contains one file and Disallow partial clearing.
  const auto sourceGroup = std::make_shared<MockedSourceGroup>();
  EXPECT_CALL(*sourceGroup, getAllSourceFilePaths).WillRepeatedly(Return(std::set{FilePath{"2.cpp"}}));
  EXPECT_CALL(*sourceGroup, allowsPartialClearing()).WillRepeatedly(Return(false));
  // And: Expect SourceGroupSettingsCppEmpty from sourceGroup
  const auto sourceGroupSettings = std::make_shared<const SourceGroupSettingsCppEmpty>("ID", nullptr);
  EXPECT_CALL(testing::Const(*sourceGroup), getSourceGroupSettings()).WillRepeatedly(Return(sourceGroupSettings));
  // And: Add the mocked sourceGroup to the project
  mProject->m_sourceGroups.push_back(sourceGroup);
  // And: User should confirm to clear
  EXPECT_CALL(*mDialogView, confirm).WillOnce(Return(1));
  EXPECT_CALL(*mDialogView, clearDialogs);
  // And: A message should be pushed.
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).WillOnce(Return());

  // When: Call the build.
  mProject->buildIndex(info, mDialogView);

  // Then: The status keep the same.
  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::NONE);
}

TEST_F(ProjectFix, buildIndex_goodCase) {
  // Given: Refresh none flags and one file to clear.
  const RefreshInfo info{{}, {}, {}, RefreshMode::AllFiles};
  // And:
  auto sourceGroup = std::make_shared<MockedSourceGroup>();
  auto sourceGroupSettingsCppEmpty = std::make_shared<const SourceGroupSettingsCppEmpty>("ID", nullptr);
  EXPECT_CALL(testing::Const(*sourceGroup), getSourceGroupSettings()).WillRepeatedly(Return(sourceGroupSettingsCppEmpty));
  mProject->m_sourceGroups.push_back(sourceGroup);
  // And:
  mProject->m_storage = std::make_shared<MockedPersistentStorage>();
  // And: A message should be pushed.
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).WillRepeatedly(Return());
  EXPECT_CALL(*mDialogView, showUnknownProgressDialog);
  EXPECT_CALL(*mStorageCache, setUseErrorCache);
  // And:
  EXPECT_CALL(*mDialogView, hideUnknownProgressDialog);
  auto mockedIndexerCommandProvider = std::make_shared<MockedIndexerCommandProvider>();
  EXPECT_CALL(*sourceGroup, getIndexerCommandProvider).WillOnce(Return(mockedIndexerCommandProvider));

  // When: Call the build.
  mProject->buildIndex(info, mDialogView);

  // Then: The status keep the same.
  EXPECT_EQ(mProject->m_refreshStage, Project::RefreshStageType::INDEXING);
}

// TODO(Hussein): The test isn't complete
TEST_F(ProjectFix, DISABLED_buildIndex_emptyFilesInSourceGroup) {
  mProject->m_storage = std::make_shared<MockedPersistentStorage>();
  auto sourceGroup = std::make_shared<MockedSourceGroup>();
  auto sourceGroupSettings = std::make_shared<const SourceGroupSettingsCppEmpty>("ID", nullptr);
  EXPECT_CALL(testing::Const(*sourceGroup), getSourceGroupSettings()).WillRepeatedly(Return(sourceGroupSettings));
  mProject->m_sourceGroups.push_back(sourceGroup);

  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, RefreshMode::AllFiles};
  mProject->buildIndex(info, mDialogView);
}
#endif

#ifndef _WIN32
class ProjectTest : public Test {
protected:
  // std::shared_ptr<MockedFileSystem> mockFileSystem;
  std::shared_ptr<MockedDialogView> mockDialogView;
  std::shared_ptr<ProjectSettings> mockSettings;
  std::shared_ptr<StorageCache> mockStorageCache;
  std::string appUUID;
  bool hasGUI;
  std::unique_ptr<Project> project;

  void SetUp() override {
    // mockFileSystem = std::make_shared<MockFileSystem>();
    mockDialogView = std::make_shared<MockedDialogView>();
    mockSettings = std::make_shared<ProjectSettings>();
    mockStorageCache = std::make_shared<StorageCache>();
    appUUID = "testUUID";
    hasGUI = true;
    project = std::make_unique<Project>(mockSettings, mockStorageCache, appUUID, hasGUI);
  }
};

TEST_F(ProjectTest, SwapToTempStorageFile_Success) {
  FilePath indexDbFilePath("index.db");
  FilePath tempIndexDbFilePath("temp_index.db");

  // EXPECT_CALL(*mockFileSystem, remove(indexDbFilePath)).Times(1);
  // EXPECT_CALL(*mockFileSystem, rename(tempIndexDbFilePath, indexDbFilePath)).Times(1);

  bool result = project->swapToTempStorageFile(indexDbFilePath, tempIndexDbFilePath, mockDialogView);
  EXPECT_TRUE(result);
}

TEST_F(ProjectTest, DISABLED_SwapToTempStorageFile_RemoveException) {
  FilePath indexDbFilePath("index.db");
  FilePath tempIndexDbFilePath("temp_index.db");

  // EXPECT_CALL(*mockFileSystem, remove(indexDbFilePath)).WillOnce(Throw(std::runtime_error("remove error")));
  EXPECT_CALL(*mockDialogView, confirm(_, _)).Times(1);

  bool result = project->swapToTempStorageFile(indexDbFilePath, tempIndexDbFilePath, mockDialogView);
  EXPECT_FALSE(result);
}

TEST_F(ProjectTest, DISABLED_SwapToTempStorageFile_RenameException) {
  FilePath indexDbFilePath("index.db");
  FilePath tempIndexDbFilePath("temp_index.db");

  // EXPECT_CALL(*mockFileSystem, remove(indexDbFilePath)).Times(1);
  // EXPECT_CALL(*mockFileSystem, rename(tempIndexDbFilePath, indexDbFilePath)).WillOnce(Throw(std::runtime_error("rename error")));
  EXPECT_CALL(*mockDialogView, confirm(_, _)).Times(1);

  bool result = project->swapToTempStorageFile(indexDbFilePath, tempIndexDbFilePath, mockDialogView);
  EXPECT_FALSE(result);
}
#endif
