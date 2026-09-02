#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "tooltip/logic/TooltipController.h"
#include "tooltip/logic/TooltipView.h"
#include "tooltip/messages/MessageTooltipHide.h"
#include "tooltip/tests/MockedTooltipView.hpp"
#include "type/MessageWindowFocus.h"

using namespace testing;

/**
 * Characterization tests for TooltipController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct TooltipControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedTooltipView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createTooltipView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createTooltipComponent(mViewLayout.get());
    mController = mComponent->getController<TooltipController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::shared_ptr<MockedTooltipView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  TooltipController* mController = nullptr;
};

TEST_F(TooltipControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<TooltipView>(), mView.get());
}

TEST_F(TooltipControllerFix, clearHidesTheTooltipUnconditionally) {
  EXPECT_CALL(*mView, hideTooltip(true)).Times(1);
  mController->clear();
}

TEST_F(TooltipControllerFix, tooltipHideMessageForcesTheTooltipDown) {
  EXPECT_CALL(*mView, hideTooltip(true)).Times(1);

  MessageTooltipHide message;
  mController->handleMessage(&message);
}

TEST_F(TooltipControllerFix, losingWindowFocusHidesTheTooltip) {
  EXPECT_CALL(*mView, hideTooltip(true)).Times(1);

  MessageWindowFocus message(false);
  mController->handleMessage(&message);
}

TEST_F(TooltipControllerFix, viewReportsItsName) {
  EXPECT_EQ(mView->getName(), "TooltipView");
}
