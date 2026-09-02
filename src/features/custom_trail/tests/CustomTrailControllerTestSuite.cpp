#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "custom_trail/logic/CustomTrailController.h"
#include "custom_trail/logic/CustomTrailView.h"
#include "custom_trail/tests/MockedCustomTrailView.hpp"
#include "data/graph/Edge.h"
#include "data/NodeTypeSet.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "type/MessageWindowClosed.h"

using namespace testing;

/**
 * Characterization tests for CustomTrailController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct CustomTrailControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedCustomTrailView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createCustomTrailView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createCustomTrailComponent(mViewLayout.get());
    mController = mComponent->getController<CustomTrailController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::shared_ptr<MockedCustomTrailView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  CustomTrailController* mController = nullptr;
};

TEST_F(CustomTrailControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<CustomTrailView>(), mView.get());
}

TEST_F(CustomTrailControllerFix, clearResetsTheViewAndRepublishesTheAvailableTypes) {
  EXPECT_CALL(*mView, clearView()).Times(1);
  EXPECT_CALL(*mStorageAccess, getAvailableNodeTypes()).WillOnce(Return(NodeKindMask{7}));
  EXPECT_CALL(*mStorageAccess, getAvailableEdgeTypes()).WillOnce(Return(Edge::TypeMask{Edge::EDGE_CALL}));
  EXPECT_CALL(*mView, setAvailableNodeAndEdgeTypes(NodeKindMask{7}, Edge::TypeMask{Edge::EDGE_CALL})).Times(1);

  mController->clear();
}

TEST_F(CustomTrailControllerFix, closingTheWindowHidesTheView) {
  EXPECT_CALL(*mView, hideView()).Times(1);

  // handleMessage is a private override; reach it the way the bus does, through the listener base.
  MessageWindowClosed message;
  static_cast<MessageListener<MessageWindowClosed>*>(mController)->handleMessageBase(&message);
}

TEST_F(CustomTrailControllerFix, viewReportsItsName) {
  EXPECT_EQ(mView->getName(), "custom trail");
}
