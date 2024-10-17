#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ComponentFactory.h"
#include "mocks/MockedStatusBarView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "StatusBarController.h"
#include "StatusView.h"


using namespace testing;

struct StatusBarControllerFix : Test {
  void SetUp() override {
    MessageQueue::getInstance()->startMessageLoopThreaded();

    MockedViewFactory viewFactory;
    storageAccess = std::make_shared<MockedStorageAccess>();
    auto componentFactory = std::make_shared<ComponentFactory>(&viewFactory, storageAccess.get());

    statusBarView = std::make_shared<MockedStatusBarView>(&viewLayout);
    EXPECT_CALL(viewFactory, createStatusBarView).WillOnce(Return(statusBarView));

    component = componentFactory->createStatusBarComponent(&viewLayout);
    controller = component->getController<StatusBarController>();
    ASSERT_NE(nullptr, controller);
  }

  void TearDown() override {
    MessageQueue::getInstance()->stopMessageLoop();
  }

  StatusBarController* controller = nullptr;
  MockedViewLayout viewLayout;
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
  MessageErrorCountClear{}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessageErrorCountUpdate) {
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  MessageErrorCountUpdate{ErrorCountInfo{}, {}}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessageIndexingFinished) {
  EXPECT_CALL(*storageAccess, getErrorCount()).WillOnce(Return(ErrorCountInfo{}));
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  EXPECT_CALL(*statusBarView, hideIndexingProgress()).WillOnce(Return());
  MessageIndexingFinished{}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessageIndexingStarted) {
  EXPECT_CALL(*statusBarView, showIndexingProgress(_)).WillOnce(Return());
  MessageIndexingStarted{}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, ShowProgressMessageIndexingStatus) {
  EXPECT_CALL(*statusBarView, showIndexingProgress(_)).WillOnce(Return());
  MessageIndexingStatus{true}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, HideProgressMessageIndexingStatus) {
  EXPECT_CALL(*statusBarView, hideIndexingProgress()).WillOnce(Return());
  MessageIndexingStatus{false}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, NoConnectionMessagePingReceived) {
  EXPECT_CALL(*statusBarView, showIdeStatus(_)).WillOnce(Return());
  MessagePingReceived{}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessagePingReceived) {
  EXPECT_CALL(*statusBarView, showIdeStatus(_)).WillOnce(Return());
  MessagePingReceived messagePingReceived;
  messagePingReceived.ideName = L"vim";
  messagePingReceived.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessageRefresh) {
  EXPECT_CALL(*statusBarView, setErrorCount(_)).WillOnce(Return());
  EXPECT_CALL(*storageAccess, getErrorCount()).WillOnce(Return(ErrorCountInfo{}));
  MessageRefresh{}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, ShowInStatusBarMessageStatus) {
  EXPECT_CALL(*statusBarView, showMessage(_, _, _)).WillOnce(Return());
  MessageStatus{L"TEST"}.dispatchImmediately();
}

TEST_F(StatusBarControllerFix, MessageStatus) {
  EXPECT_CALL(*statusBarView, showMessage(_, _, _)).Times(0);
  MessageStatus{L"", false, false, false}.dispatchImmediately();
}