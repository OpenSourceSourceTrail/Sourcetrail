#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ComponentFactory.h"
#include "mocks/MockedStatusView.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "StatusController.h"
#include "StatusView.h"

using namespace testing;

struct StatusControllerFix : Test {
  void SetUp() override {
    MessageQueue::getInstance()->startMessageLoopThreaded();

    MockedViewFactory viewFactory;
    auto componentFactory = std::make_shared<ComponentFactory>(&viewFactory, nullptr);

    statusView = std::make_shared<MockedStatusView>(&viewLayout);
    EXPECT_CALL(viewFactory, createStatusView).WillOnce(Return(statusView));

    component = componentFactory->createStatusComponent(&viewLayout);
    controller = component->getController<StatusController>();
    ASSERT_NE(nullptr, controller);
  }

  void TearDown() override {
    MessageQueue::getInstance()->stopMessageLoop();
  }

  StatusController* controller = nullptr;
  MockedViewLayout viewLayout;
  std::shared_ptr<MockedStatusView> statusView;
  std::shared_ptr<Component> component;
};

TEST_F(StatusControllerFix, MessageClearStatusView) {
  EXPECT_CALL(*statusView, clear).WillOnce(Return());
  MessageClearStatusView{}.dispatchImmediately();
}

TEST_F(StatusControllerFix, MessageShowStatus) {
  EXPECT_CALL(viewLayout, showView(_)).WillOnce(Return());
  MessageShowStatus{}.dispatchImmediately();
}

TEST_F(StatusControllerFix, EmptyMessageStatus) {
  EXPECT_CALL(*statusView, addStatus(_)).Times(0);
  MessageStatus{L""}.dispatchImmediately();
}

TEST_F(StatusControllerFix, MessageStatus) {
  EXPECT_CALL(*statusView, addStatus(_)).WillOnce(Return());
  MessageStatus{L"Message"}.dispatchImmediately();
}

TEST_F(StatusControllerFix, MessageStatusFilterChanged) {
  EXPECT_CALL(*statusView, clear).WillOnce(Return());
  EXPECT_CALL(*statusView, addStatus(_)).WillOnce(Return());
  MessageStatusFilterChanged{STATUS_INFO}.dispatch();
}
