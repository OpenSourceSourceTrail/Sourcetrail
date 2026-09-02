#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "refresh/logic/RefreshController.h"
#include "refresh/logic/RefreshView.h"
#include "refresh/tests/MockedRefreshView.hpp"

using namespace testing;

/**
 * Characterization tests for RefreshController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct RefreshControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedRefreshView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createRefreshView(mViewLayout.get())).InSequence(mSequence).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createRefreshComponent(mViewLayout.get());
    mController = mComponent->getController<RefreshController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  testing::Sequence mSequence;
  std::shared_ptr<MockedRefreshView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  RefreshController* mController = nullptr;
};

TEST_F(RefreshControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<RefreshView>(), mView.get());
}

TEST_F(RefreshControllerFix, clearTouchesNeitherViewNorStorage) {
  // Both mocks are strict, so an unexpected call on either fails the test.
  mController->clear();
}

TEST_F(RefreshControllerFix, clearIsIdempotent) {
  mController->clear();
  mController->clear();
}

TEST_F(RefreshControllerFix, viewReportsItsName) {
  EXPECT_EQ(mView->getName(), "RefreshView");
}
