#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ActivationController.h"
#include "ComponentFactory.h"
#include "IApplicationSettings.hpp"
#include "MockedApplicationSetting.hpp"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"

using namespace testing;

struct ComponentFactoryFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);
    IApplicationSettings::setInstance(mMocked);

    mFactory = std::make_unique<ComponentFactory>(&mockedViewFactory, &mockedStorageAccess);
  }

  void TearDown() override {
    mFactory.reset();
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMocked =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
  MockedViewFactory mockedViewFactory;
  MockedStorageAccess mockedStorageAccess;
  std::unique_ptr<ComponentFactory> mFactory;
};

TEST_F(ComponentFactoryFix, goodCase) {
  EXPECT_CALL(*mMocked, getStatusFilter).WillOnce(testing::Return(0));

  ASSERT_EQ(&mockedViewFactory, mFactory->getViewFactory());
  ASSERT_EQ(&mockedStorageAccess, mFactory->getStorageAccess());

  auto activationComponent = mFactory->createActivationComponent();
  EXPECT_TRUE(activationComponent);

  Sequence sequence;
  EXPECT_CALL(mockedViewFactory, createBookmarkView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto bookmarkComponent = mFactory->createBookmarkComponent(nullptr);
  EXPECT_TRUE(bookmarkComponent);

  EXPECT_CALL(mockedViewFactory, createCodeView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto codeComponent = mFactory->createCodeComponent(nullptr);
  EXPECT_TRUE(codeComponent);

  EXPECT_CALL(mockedViewFactory, createCustomTrailView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto customTrailComponent = mFactory->createCustomTrailComponent(nullptr);
  EXPECT_TRUE(customTrailComponent);

  EXPECT_CALL(mockedViewFactory, createErrorView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto errorComponent = mFactory->createErrorComponent(nullptr);
  EXPECT_TRUE(errorComponent);

  EXPECT_CALL(mockedViewFactory, createGraphView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto graphComponent = mFactory->createGraphComponent(nullptr);
  EXPECT_TRUE(graphComponent);

  EXPECT_CALL(mockedViewFactory, createRefreshView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto refreshComponent = mFactory->createRefreshComponent(nullptr);
  EXPECT_TRUE(refreshComponent);

  EXPECT_CALL(mockedViewFactory, createScreenSearchView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto screenSearchComponent = mFactory->createScreenSearchComponent(nullptr);
  EXPECT_TRUE(screenSearchComponent);

  EXPECT_CALL(mockedViewFactory, createSearchView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto searchComponent = mFactory->createSearchComponent(nullptr);
  EXPECT_TRUE(searchComponent);

  EXPECT_CALL(mockedViewFactory, createStatusBarView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto statusBarComponent = mFactory->createStatusBarComponent(nullptr);
  EXPECT_TRUE(statusBarComponent);

  EXPECT_CALL(mockedViewFactory, createStatusView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto statusComponent = mFactory->createStatusComponent(nullptr);
  EXPECT_TRUE(statusComponent);

  EXPECT_CALL(mockedViewFactory, createTabsView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto tabsComponent = mFactory->createTabsComponent(nullptr, nullptr);
  EXPECT_TRUE(tabsComponent);

  EXPECT_CALL(mockedViewFactory, createTooltipView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto tooltipComponent = mFactory->createTooltipComponent(nullptr);
  EXPECT_TRUE(tooltipComponent);

  EXPECT_CALL(mockedViewFactory, createUndoRedoView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto undoRedoComponent = mFactory->createUndoRedoComponent(nullptr);
  EXPECT_TRUE(undoRedoComponent);
}
