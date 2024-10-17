#include <memory>
#include <regex>
#include <thread>

#include <range/v3/algorithm/any_of.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gmock/gmock.h"

#ifndef D_WINDOWS
#  define private public
#endif
#include "Application.h"
#include "Project.h"
#include "ProjectSettings.h"
#ifndef D_WINDOWS
#  undef private
#endif
#include "MockedFactory.hpp"
#include "MockedProject.hpp"
#include "MockedSharedMemoryGarbageCollector.hpp"
#include "mocks/MockedApplicationSetting.hpp"
#include "mocks/MockedMessageQueue.hpp"
#include "mocks/MockedTaskManager.hpp"
#include "type/MessageStatus.h"
#include "Version.h"

using namespace std::chrono_literals;

struct MockedMessageStatus : MessageListener<MessageStatus> {
  MOCK_METHOD(void, handleMessage, (MessageStatus*), (override));
};

struct SingletonApplicationFix : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMockedAppSettings);
    IMessageQueue::setInstance(mMockedMessageQueue);

    mMockedMessageStatus = std::make_unique<testing::StrictMock<MockedMessageStatus>>();
    mTaskManager = std::make_shared<scheduling::mocks::MockedTaskManager>();
    scheduling::ITaskManager::setInstance(mTaskManager);

    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);
  }

  void TearDown() override {
    mMockedMessageStatus.reset();

    lib::ISharedMemoryGarbageCollector::setInstance(nullptr);
    mMockedSharedMemoryGarbageCollector.reset();
    IApplicationSettings::setInstance(nullptr);
    mMockedAppSettings.reset();

    scheduling::ITaskManager::setInstance(nullptr);
    mTaskManager.reset();

    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  void MockAppSettingsForGetInstance() {
    EXPECT_CALL(*mMockedAppSettings, load(testing::_, testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*mMockedAppSettings, getLoggingEnabled()).WillOnce(testing::Return(false));
    EXPECT_CALL(*mMockedAppSettings, getColorSchemePath()).WillOnce(testing::Return(std::filesystem::path{}));
  }

  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMockedAppSettings =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
  std::shared_ptr<testing::NiceMock<MockedMessageQueue>> mMockedMessageQueue =
      std::make_shared<testing::NiceMock<MockedMessageQueue>>();
  std::unique_ptr<testing::StrictMock<MockedMessageStatus>> mMockedMessageStatus;
  std::shared_ptr<testing::StrictMock<lib::MockedFactory>> mMockedFactory =
      std::make_shared<testing::StrictMock<lib::MockedFactory>>();
  std::shared_ptr<lib::MockedSharedMemoryGarbageCollector> mMockedSharedMemoryGarbageCollector =
      std::make_shared<lib::MockedSharedMemoryGarbageCollector>();
  std::shared_ptr<scheduling::mocks::MockedTaskManager> mTaskManager;
  std::shared_ptr<MockedMessageQueue> mMessageQueue;
};

TEST_F(SingletonApplicationFix, getInstanceWithoutCreate) {
#if 0    // Enabled when assert function is enabled in the getInstance function
#  ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#  else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#  endif
#endif
  // When
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
}

TEST_F(SingletonApplicationFix, singleton) {
  EXPECT_CALL(*mMockedFactory, createMessageQueue).WillOnce(testing::Return(mMessageQueue));
  EXPECT_CALL(*mMockedFactory, createTaskManager).WillOnce(testing::Return(mTaskManager));
  auto task = std::make_shared<TaskScheduler>(GlobalId{});
  EXPECT_CALL(*mTaskManager, getScheduler).WillRepeatedly(testing::Return(task));

  MockAppSettingsForGetInstance();
  EXPECT_CALL(*mMockedFactory, createSharedMemoryGarbageCollector).WillOnce(testing::Return(mMockedSharedMemoryGarbageCollector));
  EXPECT_CALL(*mMockedSharedMemoryGarbageCollector, run).WillOnce(testing::Return());
  EXPECT_CALL(*mMockedSharedMemoryGarbageCollector, stop).WillOnce(testing::Return());

  Application::createInstance(Version{}, mMockedFactory, nullptr, nullptr);
  auto baseApp = Application::getInstance();
  ASSERT_THAT(Application::getInstance(), testing::NotNull());

  auto apps = {Application::getInstance(), Application::getInstance(), Application::getInstance()};
  EXPECT_TRUE(ranges::cpp20::any_of(apps, [ptr = baseApp.get()](const auto& a) { return ptr == a.get(); }));

  // HACK: destroy before start the event-loop will start inf-loop
  std::this_thread::sleep_for(100ms);

  Application::destroyInstance();
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#if 0    // Enabled when assert function is enabled in the getInstance function
#  ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#  else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#  endif
#endif
}

TEST_F(SingletonApplicationFix, getUUID) {
  auto uuid = Application::getUUID();
  const std::regex mask{"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"};
  std::smatch base_match;
  std::regex_match(uuid, base_match, mask);
  EXPECT_EQ(1, base_match.size());
}

TEST_F(SingletonApplicationFix, loadSettingsFailedToLoad) {
  EXPECT_CALL(*mMockedAppSettings, load).WillOnce(::testing::Return(false));
  EXPECT_CALL(*mMockedAppSettings, getLoggingEnabled).WillOnce(::testing::Return(false));
  EXPECT_CALL(*mMockedAppSettings, getColorSchemePath).WillOnce(::testing::Return(std::filesystem::path{}));
  Application::loadSettings();
}

#ifndef D_WINDOWS
TEST_F(SingletonApplicationFix, getCurrentProjectPathWhileEmpty) {
  lib::ISharedMemoryGarbageCollector::setInstance(mMockedSharedMemoryGarbageCollector);
  Application app{mMockedFactory, false};
  ASSERT_THAT(app.getCurrentProjectPath().wstr(), testing::StrEq(L""));
}

TEST_F(SingletonApplicationFix, getCurrentProjectPath) {
  lib::ISharedMemoryGarbageCollector::setInstance(mMockedSharedMemoryGarbageCollector);
  Application app{mMockedFactory, false};
  auto mockedProject = std::make_shared<MockedProject>();
  app.mProject = mockedProject;
  FilePath projectPath{"somewhere"};
  EXPECT_CALL(*mockedProject, getProjectSettingsFilePath).WillOnce(testing::Return(projectPath));
  ASSERT_EQ(app.getCurrentProjectPath(), projectPath);
}

TEST_F(SingletonApplicationFix, isProjectLoadedWhileNotLoaded) {
  lib::ISharedMemoryGarbageCollector::setInstance(mMockedSharedMemoryGarbageCollector);
  Application app{mMockedFactory, false};
  auto mockedProject = std::make_shared<MockedProject>();
  EXPECT_CALL(*mockedProject, isLoaded).WillOnce(testing::Return(false));
  app.mProject = mockedProject;
  ASSERT_FALSE(app.isProjectLoaded());
}

TEST_F(SingletonApplicationFix, isProjectLoaded) {
  lib::ISharedMemoryGarbageCollector::setInstance(mMockedSharedMemoryGarbageCollector);
  Application app{mMockedFactory, false};
  auto mockedProject = std::make_shared<MockedProject>();
  EXPECT_CALL(*mockedProject, isLoaded).WillOnce(testing::Return(true));
  app.mProject = mockedProject;
  ASSERT_TRUE(app.isProjectLoaded());
}

TEST_F(SingletonApplicationFix, isProjectLoadedWhileNoProject) {
  lib::ISharedMemoryGarbageCollector::setInstance(mMockedSharedMemoryGarbageCollector);
  Application app{mMockedFactory, false};
  ASSERT_FALSE(app.isProjectLoaded());
}
#endif

struct ApplicationFix : SingletonApplicationFix {
  void SetUp() override {
    SingletonApplicationFix::SetUp();

    MockAppSettingsForGetInstance();

    EXPECT_CALL(*mMockedFactory, createSharedMemoryGarbageCollector).WillOnce(testing::Return(mMockedSharedMemoryGarbageCollector));
    EXPECT_CALL(*mMockedSharedMemoryGarbageCollector, run).WillOnce(testing::Return());
    EXPECT_CALL(*mMockedSharedMemoryGarbageCollector, stop).WillOnce(testing::Return());

    EXPECT_CALL(*mMockedFactory, createMessageQueue).WillOnce(testing::Return(mMessageQueue));
    EXPECT_CALL(*mMockedFactory, createTaskManager).WillOnce(testing::Return(mTaskManager));
    auto task = std::make_shared<TaskScheduler>(GlobalId{});
    EXPECT_CALL(*mTaskManager, getScheduler).WillRepeatedly(testing::Return(task));
    Application::createInstance(Version{}, mMockedFactory, nullptr, nullptr);
    mApp = Application::getInstance();
  }
  void TearDown() override {
    // HACK: destroy before start the event-loop will start inf-loop
    std::this_thread::sleep_for(1ms);
    Application::destroyInstance();

    mApp.reset();

    SingletonApplicationFix::TearDown();
  }
  std::shared_ptr<Application> mApp;
  std::shared_ptr<lib::MockedFactory> mMockedFactory = std::make_shared<lib::MockedFactory>();
};

TEST_F(ApplicationFix, getCurrentProject) {
  EXPECT_THAT(mApp->getCurrentProject(), testing::IsNull());
}

TEST_F(ApplicationFix, hasGUI) {
  EXPECT_FALSE(mApp->hasGUI());
}

TEST_F(ApplicationFix, getDialogView) {
  auto view = mApp->getDialogView(DialogView::UseCase::GENERAL);
  ASSERT_THAT(view, testing::Not(testing::IsNull()));
}

#ifndef D_WINDOWS
TEST_F(ApplicationFix, LoadProjectMessageIsNull) {
  // Given: Application is created
  ASSERT_THAT(mApp, testing::NotNull());

  // When: Load a new project
  MessageLoadProject* message = nullptr;
  EXPECT_NO_FATAL_FAILURE(mApp->handleMessage(message));

  // Then:
}

TEST_F(ApplicationFix, LoadProjectEmptyProjectSettings) {
  // Given: Application is created
  ASSERT_THAT(mApp, testing::NotNull());

  // When: Load a new project
  MessageLoadProject message{FilePath{L""}};
  mApp->handleMessage(&message);

  // Then:
}

TEST_F(ApplicationFix, LoadProjectProjectSettings) {
  // Given: Application is created
  ASSERT_THAT(mApp, testing::NotNull());

  auto project = std::make_shared<MockedProject>();
  EXPECT_CALL(*mMockedFactory, createProject).WillOnce(testing::Return(project));
  EXPECT_CALL(*project, load).WillOnce(testing::Return());

  // When: Load a new project
  MessageLoadProject message{FilePath{L"Something"}};
  mApp->handleMessage(&message);

  // Then:
}

TEST_F(ApplicationFix, LoadProjectWhileIndexing) {
  // Given: Application is created
  ASSERT_THAT(mApp, testing::NotNull());

  // And: A project exists
  auto project = std::make_shared<MockedProject>();
  EXPECT_CALL(*project, isIndexing).WillOnce(testing::Return(true));
  // And: The project refreshing
  mApp->mProject = std::move(project);

  // When: Load a new project
  MessageLoadProject message{FilePath{"Something"}};
  mApp->handleMessage(&message);
  // Then:
}
#endif