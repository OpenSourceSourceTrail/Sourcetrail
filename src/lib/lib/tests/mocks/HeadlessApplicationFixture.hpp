#pragma once
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/Application.h"
#include "MockedFactory.hpp"
#include "mocks/MockedApplicationSetting.hpp"
#include "mocks/MockedMessageQueue.hpp"
#include "Version.h"

/**
 * A test fixture that stands a headless Application singleton up around the usual controller mocks.
 *
 * TabsController and UndoRedoController both reach through Application::getInstance() on every path
 * -- including clear() -- so they cannot be characterized without one. Created with a null
 * ViewFactory, so getDialogView() and updateHistoryMenu() no-op rather than needing Qt.
 *
 * Closing that reach-through is Phase 4 of the three-tier plan; once controllers take what they
 * need by injection, the suites built on this fixture can drop it.
 */
struct HeadlessApplicationFixture : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mAppSettings);
    EXPECT_CALL(*mAppSettings, load(testing::_, testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mAppSettings, getLoggingEnabled()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mAppSettings, getColorSchemePath()).WillRepeatedly(testing::Return(std::filesystem::path{}));

    mMessageQueue = std::make_shared<testing::NiceMock<MockedMessageQueue>>();
    ON_CALL(*mMessageQueue, pushMessage(testing::_)).WillByDefault([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });
    ON_CALL(*mMessageQueue, processMessage(testing::_, testing::_))
        .WillByDefault([this](const std::shared_ptr<MessageBase>& message, bool) { mDispatched.push_back(message->getType()); });

    EXPECT_CALL(*mFactory, createSharedMemoryGarbageCollector()).WillOnce(testing::Return(nullptr));
    EXPECT_CALL(*mFactory, createMessageQueue()).WillOnce(testing::Return(mMessageQueue));

    Application::createInstance(Version{}, mFactory, nullptr, nullptr);
    ASSERT_THAT(Application::getInstance(), testing::NotNull());
  }

  void TearDown() override {
    Application::destroyInstance();

    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();

    IApplicationSettings::setInstance(nullptr);
    mAppSettings.reset();
  }

  std::shared_ptr<testing::NiceMock<MockedApplicationSettings>> mAppSettings =
      std::make_shared<testing::NiceMock<MockedApplicationSettings>>();
  std::shared_ptr<testing::NiceMock<lib::MockedFactory>> mFactory = std::make_shared<testing::NiceMock<lib::MockedFactory>>();
  std::shared_ptr<testing::NiceMock<MockedMessageQueue>> mMessageQueue;
  std::vector<std::string> mDispatched;
};
