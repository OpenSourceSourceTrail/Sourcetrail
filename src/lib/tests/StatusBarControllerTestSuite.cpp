#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ComponentFactory.h"
#include "MessageQueue.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStatusBarView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#define private public
#include "StatusBarController.h"
#undef private
#include "StatusView.h"


using namespace testing;

struct StatusBarControllerFix : Test {
  void SetUp() override {
    messageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(messageQueue);

    MockedViewFactory viewFactory;
    storageAccess = std::make_shared<MockedStorageAccess>();
    auto componentFactory = std::make_shared<ComponentFactory>(&viewFactory, storageAccess.get());

    statusBarView = std::make_shared<MockedStatusBarView>(&viewLayout);
    EXPECT_CALL(viewFactory, createStatusBarView).WillOnce(Return(statusBarView));

    component = componentFactory->createStatusBarComponent(&viewLayout);
    controller = component->getController<StatusBarController>();
    ASSERT_NE(nullptr, controller);
  }

  void TearDown() override {}

  StatusBarController* controller = nullptr;
  MockedViewLayout viewLayout;
  std::shared_ptr<MockedMessageQueue> messageQueue;
  std::shared_ptr<MockedStatusBarView> statusBarView;
  std::shared_ptr<Component> component;
  std::shared_ptr<MockedStorageAccess> storageAccess;
};

TEST_F(StatusBarControllerFix, getView) {
  auto* view = controller->getView();
  EXPECT_EQ(statusBarView.get(), view);
}

TEST_F(StatusBarControllerFix, clear) {
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  controller->clear();
}

TEST_F(StatusBarControllerFix, MessageErrorCountClear) {
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  MessageErrorCountClear message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessageErrorCountUpdate) {
  const ErrorCountInfo info{1, 2};
  EXPECT_CALL(*statusBarView, setErrorCount(info)).WillOnce(Return());
  MessageErrorCountUpdate message{info, std::vector<ErrorInfo>{}};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessageIndexingFinished) {
  const ErrorCountInfo info{1, 2};
  EXPECT_CALL(*storageAccess, getErrorCount()).WillOnce(Return(info));
  EXPECT_CALL(*statusBarView, setErrorCount(info)).WillOnce(Return());
  EXPECT_CALL(*statusBarView, hideIndexingProgress).WillOnce(Return());
  MessageIndexingFinished message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessageIndexingStarted) {
  EXPECT_CALL(*statusBarView, showIndexingProgress(_)).WillOnce(Return());
  MessageIndexingStarted message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, ShowProgressMessageIndexingStatus) {
  EXPECT_CALL(*statusBarView, showIndexingProgress(_)).WillOnce(Return());
  MessageIndexingStarted message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, HideProgressMessageIndexingStatus) {
  EXPECT_CALL(*statusBarView, hideIndexingProgress()).WillOnce(Return());
  MessageIndexingStatus message{false};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, NoConnectionMessagePingReceived) {
  EXPECT_CALL(*statusBarView, showIdeStatus(_)).WillOnce(Return());
  MessagePingReceived message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessagePingReceived) {
  EXPECT_CALL(*statusBarView, showIdeStatus(_)).WillOnce(Return());
  MessagePingReceived message{};
  message.ideName = L"vim";
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessageRefresh) {
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  EXPECT_CALL(*storageAccess, getErrorCount()).WillOnce(Return(ErrorCountInfo{}));
  MessageRefresh message{};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, ShowInStatusBarMessageStatus) {
  EXPECT_CALL(*statusBarView, showMessage(_, _, _)).WillOnce(Return());
  MessageStatus message{L"TEST"};
  controller->handleMessage(&message);
}

TEST_F(StatusBarControllerFix, MessageStatus) {
  EXPECT_CALL(*statusBarView, showMessage(_, _, _)).Times(0);
  MessageStatus message{L"", false, false, false};
  controller->handleMessage(&message);
}
