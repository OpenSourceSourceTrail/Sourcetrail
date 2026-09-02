#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "mocks/HeadlessApplicationFixture.hpp"
#include "mocks/MockedScreenSearchSender.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "tabs/logic/TabsController.h"
#include "tabs/logic/TabsView.h"
#include "tabs/tests/MockedTabsView.hpp"

using namespace testing;

/**
 * Characterization tests for TabsController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 *
 * Every tab-opening path is gated on Application::getInstance()->isProjectLoaded(), hence the
 * HeadlessApplicationFixture. No project is loaded in these tests, so those paths take their
 * "no project" branch -- which is exactly what is pinned here.
 */
struct TabsControllerFix : HeadlessApplicationFixture {
  void SetUp() override {
    HeadlessApplicationFixture::SetUp();

    mViewLayout = std::make_unique<NiceMock<MockedViewLayout>>();
    mView = std::make_shared<MockedTabsView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createTabsView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createTabsComponent(mViewLayout.get(), &mScreenSearchSender);
    mController = mComponent->getController<TabsController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    HeadlessApplicationFixture::TearDown();
  }

  template <typename MessageType>
  void deliver(MessageType& message) {
    static_cast<MessageListener<MessageType>*>(mController)->handleMessageBase(&message);
  }

  MockedScreenSearchSender mScreenSearchSender;
  std::shared_ptr<MockedTabsView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<NiceMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  TabsController* mController = nullptr;
};

TEST_F(TabsControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<TabsView>(), mView.get());
}

TEST_F(TabsControllerFix, clearReturnsImmediatelyWhenThereAreNoTabs) {
  // clear() busy-waits until m_tabs drains; with no tabs it clears the view and returns at once.
  EXPECT_CALL(*mView, clear()).Times(1);

  mController->clear();
}

TEST_F(TabsControllerFix, closingATabIsForwardedToTheViewUnconditionally) {
  EXPECT_CALL(*mView, closeTab()).Times(1);

  MessageTabClose message;
  deliver(message);
}

TEST_F(TabsControllerFix, selectingATabIsForwardedToTheView) {
  EXPECT_CALL(*mView, selectTab(true)).Times(1);

  MessageTabSelect message(true);
  deliver(message);
}

TEST_F(TabsControllerFix, tabStateIsForwardedToTheView) {
  const std::vector<SearchMatch> matches;
  EXPECT_CALL(*mView, updateTab(7, matches)).Times(1);

  MessageTabState message(7, matches);
  deliver(message);
}

TEST_F(TabsControllerFix, openingATabIsRefusedWhileNoProjectIsLoaded) {
  // The strict view mock is what asserts openTab() is never reached.
  MessageTabOpen message;
  deliver(message);
}

TEST_F(TabsControllerFix, openWithIsRefusedWhileNoProjectIsLoadedBeforeTouchingStorage) {
  // The strict storage mock asserts the lookup chain is never entered.
  MessageTabOpenWith message(FilePath(L"/tmp/foo.cpp"), 12);
  deliver(message);
}

TEST_F(TabsControllerFix, activatingErrorsWithoutAProjectOpensNothing) {
  MessageActivateErrors message{ErrorFilter()};
  deliver(message);

  EXPECT_THAT(mDispatched, Not(Contains("MessageTabOpenWith")));
}

TEST_F(TabsControllerFix, indexingFinishedWithoutAProjectOpensNothing) {
  MessageIndexingFinished message;
  deliver(message);

  EXPECT_THAT(mDispatched, Not(Contains("MessageTabOpenWith")));
}
