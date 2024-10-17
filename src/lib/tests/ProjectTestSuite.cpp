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
#include "mocks/MockedDialogView.hpp"
#include "mocks/MockedMessageQueue.hpp"
#include "mocks/MockedProjectSettings.hpp"
#include "mocks/MockedStorageCache.hpp"

using namespace std::chrono_literals;
using namespace testing;

constexpr auto WaitingDuration = 100ms;

struct MockedMessageStatus : MessageListener<MessageStatus> {
  MOCK_METHOD(void, handleMessage, (MessageStatus*), (override));
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
  }

  void TearDown() override {
    IMessageQueue::setInstance(nullptr);
    mMockedMessageQueue.reset();
  }

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

TEST_F(ProjectFix, loadGoodCase) {
  EXPECT_CALL(*mMockedMessageQueue, pushMessage).Times(2);
  EXPECT_CALL(*mSettings, load(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mStorageCache, setUseErrorCache(_)).WillOnce(Return());

  mProject->load(mDialogView);

  std::this_thread::sleep_for(WaitingDuration);

  EXPECT_TRUE(mProject->isLoaded());
  EXPECT_FALSE(mProject->isIndexing());
}

TEST_F(ProjectFix, DISABLED_loadFailed) {
  EXPECT_CALL(*mSettings, load(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mStorageCache, setUseErrorCache(_)).WillOnce(Return());

  // mProject->m_refreshStage = Project::RefreshStageType::NONE;
  mProject->load(mDialogView);

  EXPECT_TRUE(mProject->isLoaded());
  EXPECT_FALSE(mProject->isIndexing());
}

TEST_F(ProjectFix, DISABLED_buildIndex) {
  EXPECT_CALL(*mDialogView, clearDialogs()).WillOnce(Return());

  const RefreshInfo info{{FilePath{"1.cpp"}}, {}, {}, REFRESH_ALL_FILES};
  mProject->buildIndex(info, mDialogView);
}
