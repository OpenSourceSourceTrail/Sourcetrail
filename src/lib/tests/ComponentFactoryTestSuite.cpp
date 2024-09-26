#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ActivationController.h"
#include "ComponentFactory.h"
#include "IApplicationSettings.hpp"
#include "MockedApplicationSetting.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"

using namespace testing;

struct ComponentFactoryFix : Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
  }

  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
  }

  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMocked =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
  MockedViewFactory mockedViewFactory;
  MockedStorageAccess mockedStorageAccess;
  ComponentFactory factory{&mockedViewFactory, &mockedStorageAccess};
};

TEST_F(ComponentFactoryFix, goodCase) {
  EXPECT_CALL(*mMocked, getStatusFilter).WillOnce(testing::Return(0));

  ASSERT_EQ(&mockedViewFactory, factory.getViewFactory());
  ASSERT_EQ(&mockedStorageAccess, factory.getStorageAccess());

  auto activationComponent = factory.createActivationComponent();
  EXPECT_TRUE(activationComponent);

  Sequence sequence;
  EXPECT_CALL(mockedViewFactory, createBookmarkView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto bookmarkComponent = factory.createBookmarkComponent(nullptr);
  EXPECT_TRUE(bookmarkComponent);

  EXPECT_CALL(mockedViewFactory, createCodeView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto codeComponent = factory.createCodeComponent(nullptr);
  EXPECT_TRUE(codeComponent);

  EXPECT_CALL(mockedViewFactory, createCustomTrailView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto customTrailComponent = factory.createCustomTrailComponent(nullptr);
  EXPECT_TRUE(customTrailComponent);

  EXPECT_CALL(mockedViewFactory, createErrorView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto errorComponent = factory.createErrorComponent(nullptr);
  EXPECT_TRUE(errorComponent);

  EXPECT_CALL(mockedViewFactory, createGraphView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto graphComponent = factory.createGraphComponent(nullptr);
  EXPECT_TRUE(graphComponent);

  EXPECT_CALL(mockedViewFactory, createRefreshView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto refreshComponent = factory.createRefreshComponent(nullptr);
  EXPECT_TRUE(refreshComponent);

  EXPECT_CALL(mockedViewFactory, createScreenSearchView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto screenSearchComponent = factory.createScreenSearchComponent(nullptr);
  EXPECT_TRUE(screenSearchComponent);

  EXPECT_CALL(mockedViewFactory, createSearchView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto searchComponent = factory.createSearchComponent(nullptr);
  EXPECT_TRUE(searchComponent);

  EXPECT_CALL(mockedViewFactory, createStatusBarView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto statusBarComponent = factory.createStatusBarComponent(nullptr);
  EXPECT_TRUE(statusBarComponent);

  EXPECT_CALL(mockedViewFactory, createStatusView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto statusComponent = factory.createStatusComponent(nullptr);
  EXPECT_TRUE(statusComponent);

  EXPECT_CALL(mockedViewFactory, createTabsView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto tabsComponent = factory.createTabsComponent(nullptr, nullptr);
  EXPECT_TRUE(tabsComponent);

  EXPECT_CALL(mockedViewFactory, createTooltipView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto tooltipComponent = factory.createTooltipComponent(nullptr);
  EXPECT_TRUE(tooltipComponent);

  EXPECT_CALL(mockedViewFactory, createUndoRedoView(_)).InSequence(sequence).WillOnce(Return(nullptr));
  auto undoRedoComponent = factory.createUndoRedoComponent(nullptr);
  EXPECT_TRUE(undoRedoComponent);
}
