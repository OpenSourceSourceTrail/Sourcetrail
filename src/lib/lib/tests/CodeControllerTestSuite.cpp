#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "activation/messages/MessageActivateLegend.h"
#include "component/ComponentFactory.h"
#include "component/controller/CodeController.h"
#include "component/view/CodeView.h"
#include "error/messages/MessageErrorCountClear.h"
#include "error/messages/MessageShowError.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedCodeView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "type/code/MessageScrollCode.h"
#include "type/code/MessageScrollToLine.h"
#include "type/focus/MessageFocusIn.h"
#include "type/focus/MessageFocusOut.h"

using namespace testing;

/**
 * Characterization tests for CodeController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 *
 * The heavy activation paths (MessageActivateTokens, MessageActivateOverview, ...) are not covered
 * here: MessageActivateOverview and getProjectDescription() both call Application::getInstance(),
 * so they cannot run without a live Application. Closing that reach-through is Phase 4 of the plan.
 */
struct CodeControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, pushMessage(_)).Times(AnyNumber()).WillRepeatedly([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedCodeView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createCodeView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createCodeComponent(mViewLayout.get());
    mController = mComponent->getController<CodeController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  template <typename MessageType>
  void deliver(MessageType& message) {
    static_cast<MessageListener<MessageType>*>(mController)->handleMessageBase(&message);
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::vector<std::string> mDispatched;
  std::shared_ptr<MockedCodeView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  CodeController* mController = nullptr;
};

TEST_F(CodeControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<CodeView>(), mView.get());
}

TEST_F(CodeControllerFix, schedulerIdIsTheTabId) {
  EXPECT_EQ(mController->getSchedulerId(), mComponent->getTabId());
}

TEST_F(CodeControllerFix, clearOnlyClearsTheView) {
  EXPECT_CALL(*mView, clear()).Times(1);

  static_cast<Controller*>(mController)->clear();
}

TEST_F(CodeControllerFix, activatingTheLegendClearsTheCodeView) {
  EXPECT_CALL(*mView, clear()).Times(1);

  MessageActivateLegend message;
  deliver(message);
}

TEST_F(CodeControllerFix, clearingTheErrorCountOnlyClearsWhileTheViewShowsErrors) {
  EXPECT_CALL(*mView, showsErrors()).WillOnce(Return(true));
  EXPECT_CALL(*mView, clear()).Times(1);

  MessageErrorCountClear message;
  deliver(message);
}

TEST_F(CodeControllerFix, clearingTheErrorCountIsANoOpWhenTheViewShowsCode) {
  EXPECT_CALL(*mView, showsErrors()).WillOnce(Return(false));

  MessageErrorCountClear message;
  deliver(message);
}

TEST_F(CodeControllerFix, focusInCoFocusesTheTokensOnTheView) {
  const std::vector<Id> tokenIds{1, 2, 3};
  EXPECT_CALL(*mView, coFocusTokenIds(tokenIds)).Times(1);

  MessageFocusIn message(tokenIds);
  deliver(message);
}

TEST_F(CodeControllerFix, focusOutDeCoFocuses) {
  EXPECT_CALL(*mView, deCoFocusTokenIds()).Times(1);

  MessageFocusOut message({});
  deliver(message);
}

TEST_F(CodeControllerFix, scrollToLineScrollsWithoutAnimationAndReportsStatus) {
  EXPECT_CALL(*mView, scrollTo(_, false)).Times(1);

  MessageScrollToLine message(FilePath(L"/tmp/foo.cpp"), 42);
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
}

TEST_F(CodeControllerFix, aLiveScrollMessageIsRecordedButChangesNothing) {
  // Only a replayed scroll updates m_scrollParams; the strict view mock asserts nothing is drawn.
  MessageScrollCode message(120, true);
  deliver(message);
}

TEST_F(CodeControllerFix, showErrorIsIgnoredWhileTheViewShowsCode) {
  EXPECT_CALL(*mView, showsErrors()).WillOnce(Return(false));

  MessageShowError message(7);
  deliver(message);
}
