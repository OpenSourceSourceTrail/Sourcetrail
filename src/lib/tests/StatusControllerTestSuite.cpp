#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ComponentFactory.h"
#include "MockedApplicationSetting.hpp"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStatusView.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "Status.h"
#define private public
#include "StatusController.h"
#undef private
#include "StatusView.h"

using namespace testing;

struct StatusControllerFix : Test {
  void SetUp() override {
    mApplicationSettings = std::make_shared<MockedApplicationSettings>();
    IApplicationSettings::setInstance(mApplicationSettings);
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    ON_CALL(*mApplicationSettings, getStatusFilter).WillByDefault(Return(0U));

    MockedViewFactory viewFactory;
    auto componentFactory = std::make_shared<ComponentFactory>(&viewFactory, nullptr);

    statusView = std::make_shared<MockedStatusView>(&viewLayout);
    EXPECT_CALL(viewFactory, createStatusView).WillOnce(Return(statusView));

    component = componentFactory->createStatusComponent(&viewLayout);
    controller = component->getController<StatusController>();
    ASSERT_NE(nullptr, controller);
  }

  void TearDown() override {
    component.reset();
    statusView.reset();
    IApplicationSettings::setInstance(nullptr);
    IMessageQueue::setInstance(nullptr);
  }


  std::shared_ptr<MockedApplicationSettings> mApplicationSettings;
  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  StatusController* controller = nullptr;
  MockedViewLayout viewLayout;
  std::shared_ptr<MockedStatusView> statusView;
  std::shared_ptr<Component> component;
};

TEST_F(StatusControllerFix, getView) {
  const auto* view = controller->getView();
  ASSERT_NE(nullptr, view);
  ASSERT_EQ(statusView.get(), view);
}

TEST_F(StatusControllerFix, MessageClearStatusView) {
  EXPECT_CALL(*statusView, clear).WillOnce(Return());
  MessageClearStatusView message{};
  controller->handleMessage(&message);
}

TEST_F(StatusControllerFix, MessageShowStatus) {
  EXPECT_CALL(viewLayout, showView(_)).WillOnce(Return());
  MessageShowStatus message{};
  controller->handleMessage(&message);
}

TEST_F(StatusControllerFix, EmptyMessageStatus) {
  EXPECT_CALL(*statusView, addStatus(_)).Times(0);
  MessageStatus message{L""};
  controller->handleMessage(&message);
}

TEST_F(StatusControllerFix, MessageStatus) {
  EXPECT_CALL(*statusView, addStatus(_)).WillOnce(Return());
  MessageStatus message{L"Message"};
  controller->handleMessage(&message);
}

TEST_F(StatusControllerFix, MessageStatusFilterChanged) {
  EXPECT_CALL(*statusView, clear).WillOnce(Return());
  EXPECT_CALL(*statusView, addStatus(_)).WillOnce(Return());
  MessageStatusFilterChanged message{static_cast<int>(StatusType::Info)};
  controller->handleMessage(&message);
}
