#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "activation/messages/MessageActivateErrors.h"
#include "activation/messages/MessageActivateFullTextSearch.h"
#include "component/ComponentFactory.h"
#include "graph/logic/GraphController.h"
#include "graph/logic/GraphView.h"
#include "graph/messages/MessageDeactivateEdge.h"
#include "graph/messages/MessageScrollGraph.h"
#include "graph/tests/MockedGraphView.hpp"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "type/focus/MessageFocusIn.h"
#include "type/focus/MessageFocusOut.h"

using namespace testing;

/**
 * Characterization tests for GraphController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 *
 * The layout-producing paths (MessageActivateTokens, MessageActivateTrail, the node expand/bundle
 * handlers) are not covered here: they need a populated Graph and GraphViewStyle metrics, and
 * MessageActivateTrail calls Application::getInstance()->handleDialog(). The headless layout path
 * those exercise already has coverage through src/app/engine's GraphLayoutService.
 */
struct GraphControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, pushMessage(_)).Times(AnyNumber()).WillRepeatedly([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedGraphView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createGraphView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createGraphComponent(mViewLayout.get());
    mController = mComponent->getController<GraphController>();
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
  std::shared_ptr<MockedGraphView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  GraphController* mController = nullptr;
};

TEST_F(GraphControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<GraphView>(), mView.get());
}

TEST_F(GraphControllerFix, schedulerIdIsTheTabId) {
  EXPECT_EQ(mController->getSchedulerId(), mComponent->getTabId());
}

TEST_F(GraphControllerFix, clearOnlyClearsTheView) {
  EXPECT_CALL(*mView, clear()).Times(1);

  static_cast<Controller*>(mController)->clear();
}

TEST_F(GraphControllerFix, activatingErrorsClearsTheGraph) {
  EXPECT_CALL(*mView, clear()).Times(1);

  MessageActivateErrors message{ErrorFilter()};
  deliver(message);
}

TEST_F(GraphControllerFix, activatingFullTextSearchClearsTheGraph) {
  EXPECT_CALL(*mView, clear()).Times(1);

  MessageActivateFullTextSearch message(L"needle", false);
  deliver(message);
}

TEST_F(GraphControllerFix, deactivatingAnEdgeClearsTheActiveEdgeOnTheView) {
  EXPECT_CALL(*mView, activateEdge(0)).Times(1);

  MessageDeactivateEdge message(false);
  deliver(message);
}

TEST_F(GraphControllerFix, focusInCoFocusesTheTokensOnTheView) {
  const std::vector<Id> tokenIds{4, 5};
  EXPECT_CALL(*mView, coFocusTokenIds(tokenIds)).Times(1);

  MessageFocusIn message(tokenIds);
  deliver(message);
}

TEST_F(GraphControllerFix, focusOutDeCoFocusesTheSameTokens) {
  const std::vector<Id> tokenIds{4, 5};
  EXPECT_CALL(*mView, deCoFocusTokenIds(tokenIds)).Times(1);

  MessageFocusOut message(tokenIds);
  deliver(message);
}

TEST_F(GraphControllerFix, onlyAReplayedScrollReachesTheView) {
  MessageScrollGraph live(10, 20);
  deliver(live);    // strict view mock: a live scroll must not draw

  MessageScrollGraph replayed(10, 20);
  replayed.setIsReplayed(true);
  EXPECT_CALL(*mView, scrollToValues(10, 20)).Times(1);
  deliver(replayed);
}
