#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/Component.h"
#include "mocks/MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "search/tests/MockedScreenSearchSender.hpp"
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
 * Every tab-opening path is gated on whether a project is loaded. That used to be
 * Application::getInstance()->isProjectLoaded(), which made the controller unconstructible without
 * a whole Application; it is now injected, so `mProjectLoaded` decides it and both branches are
 * reachable from a test.
 */
struct TabsControllerFix : Test {
  void SetUp() override {
    // The mocked queue records dispatches rather than delivering them -- a MessageListener created
    // inside a test never fires -- so assertions are made on what reached the queue.
    mMessageQueue = std::make_shared<NiceMock<MockedMessageQueue>>();
    ON_CALL(*mMessageQueue, pushMessage(_)).WillByDefault([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });
    ON_CALL(*mMessageQueue, processMessage(_, _)).WillByDefault([this](const std::shared_ptr<MessageBase>& message, bool) {
      mDispatched.push_back(message->getType());
    });
    IMessageQueue::setInstance(mMessageQueue);

    mViewLayout = std::make_unique<NiceMock<MockedViewLayout>>();
    mView = std::make_shared<MockedTabsView>(mViewLayout.get());
    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();

    auto controller = std::make_shared<TabsController>(
        mViewLayout.get(), nullptr, mStorageAccess.get(), &mScreenSearchSender, [this] { return mProjectLoaded; });
    mController = controller.get();
    mComponent = std::make_shared<Component>(mView, std::move(controller));
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

  bool mProjectLoaded = false;
  std::shared_ptr<NiceMock<MockedMessageQueue>> mMessageQueue;
  std::vector<std::string> mDispatched;
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

// These two were unreachable while the gate was Application::getInstance()->isProjectLoaded():
// standing a real Application up in a test does not give you one with a project loaded.

TEST_F(TabsControllerFix, activatingErrorsWithAProjectOpensAnErrorTab) {
  mProjectLoaded = true;

  MessageActivateErrors message{ErrorFilter()};
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageTabOpenWith"));
}

TEST_F(TabsControllerFix, indexingFinishedWithAProjectOpensATab) {
  mProjectLoaded = true;

  MessageIndexingFinished message;
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageTabOpenWith"));
}
