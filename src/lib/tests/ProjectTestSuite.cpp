#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FilePath.h"
#include "MessageListener.h"
#include "type/MessageStatus.h"
#define private public
#include "Project.h"
#undef private
#include "IApplicationSettings.hpp"
#include "IndexerCommandProvider.h"
#include "ITaskManager.hpp"
#include "MockedApplicationSetting.hpp"
#include "mocks/MockedDialogView.hpp"
#include "mocks/MockedMessageQueue.hpp"
#include "mocks/MockedProjectSettings.hpp"
#include "mocks/MockedSourceGroup.hpp"
#include "mocks/MockedStorageCache.hpp"
#include "mocks/MockedTaskManager.hpp"
#include "PersistentStorage.h"
#include "SourceGroupSettings.h"
#include "SourceGroupSettingsCppEmpty.h"
#include "TabId.h"

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

struct ProjectFix : Test {
  void SetUp() override {
    mMockedMessageQueue = std::make_shared<StrictMock<MockedMessageQueue>>();
    IMessageQueue::setInstance(mMockedMessageQueue);

    mStatus = std::unique_ptr<MockedMessageStatus>();

    mSettings = std::make_shared<StrictMock<MockedProjectSettings>>();
    mStorageCache = std::make_unique<StrictMock<MockedStorageCache>>();
    mProject = std::make_unique<Project>(mSettings, mStorageCache.get(), "", true);

    mDialogView = std::make_shared<StrictMock<MockedDialogView>>();

    IApplicationSettings::setInstance(std::make_shared<MockedApplicationSettings>());
    mTaskManager = std::make_shared<scheduling::mocks::MockedTaskManager>();
    scheduling::ITaskManager::setInstance(mTaskManager);
  }

  void TearDown() override {
    scheduling::ITaskManager::setInstance(nullptr);
    IApplicationSettings::setInstance(nullptr);
    IMessageQueue::setInstance(nullptr);
    mMockedMessageQueue.reset();
  }

  std::shared_ptr<scheduling::mocks::MockedTaskManager> mTaskManager;
  std::shared_ptr<StrictMock<MockedMessageQueue>> mMockedMessageQueue;
  std::shared_ptr<StrictMock<MockedProjectSettings>> mSettings;
  std::unique_ptr<StrictMock<MockedStorageCache>> mStorageCache;
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

#if BUILD_CXX_LANGUAGE_PACKAGE
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
  const auto taskScheduler = std::make_shared<TaskScheduler>(TabId::app());
  // EXPECT_CALL(*mTaskManager, createScheduler(testing::_)).WillOnce(Return(taskScheduler));
  EXPECT_CALL(*mTaskManager, getScheduler(testing::_)).WillOnce(Return(taskScheduler));
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
  std::ignore = scheduling::ITaskManager::getInstanceRaw()->createScheduler(TabId::app());

  mProject->m_storage = std::make_shared<MockedPersistentStorage>();
  auto sourceGroup = std::make_shared<MockedSourceGroup>();
  auto sourceGroupSettings = std::make_shared<const SourceGroupSettingsCppEmpty>("ID", nullptr);
  EXPECT_CALL(testing::Const(*sourceGroup), getSourceGroupSettings()).WillRepeatedly(Return(sourceGroupSettings));
  mProject->m_sourceGroups.push_back(sourceGroup);

  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, RefreshMode::AllFiles};
  mProject->buildIndex(info, mDialogView);
}
#endif

class ProjectTest : public Test {
protected:
  // std::shared_ptr<MockedFileSystem> mockFileSystem;
  std::shared_ptr<MockedDialogView> mockDialogView;
  std::shared_ptr<ProjectSettings> mockSettings;
  StorageCache* mockStorageCache;
  std::string appUUID;
  bool hasGUI;
  std::unique_ptr<Project> project;

  void SetUp() override {
    // mockFileSystem = std::make_shared<MockFileSystem>();
    mockDialogView = std::make_shared<MockedDialogView>();
    mockSettings = std::make_shared<ProjectSettings>();
    mockStorageCache = nullptr;
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
