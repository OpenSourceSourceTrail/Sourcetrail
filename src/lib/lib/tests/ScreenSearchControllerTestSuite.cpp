#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "component/controller/helper/ScreenSearchInterfaces.h"
#include "component/controller/ScreenSearchController.h"
#include "component/view/ScreenSearchView.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedScreenSearchView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "type/code/MessageChangeFileView.h"

using namespace testing;

namespace {

struct MockedResponder : ScreenSearchResponder {
  explicit MockedResponder(std::string name) : mName(std::move(name)) {}

  [[nodiscard]] std::string getName() const override {
    return mName;
  }

  MOCK_METHOD(bool, isVisible, (), (const, override));
  MOCK_METHOD(void, findMatches, (ScreenSearchSender*, const std::wstring&), (override));
  MOCK_METHOD(void, activateMatch, (size_t), (override));
  MOCK_METHOD(void, deactivateMatch, (size_t), (override));
  MOCK_METHOD(void, clearMatches, (), (override));

  std::string mName;
};

}    // namespace

/**
 * Characterization tests for ScreenSearchController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct ScreenSearchControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedScreenSearchView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createScreenSearchView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createScreenSearchComponent(mViewLayout.get());
    mController = mComponent->getController<ScreenSearchController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::shared_ptr<MockedScreenSearchView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  ScreenSearchController* mController = nullptr;
};

TEST_F(ScreenSearchControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<ScreenSearchView>(), mView.get());
}

TEST_F(ScreenSearchControllerFix, clearIsANoOp) {
  // The view mock is strict, so any call on it would fail this test.
  mController->clear();
}

TEST_F(ScreenSearchControllerFix, addingAResponderRegistersItsNameWithTheView) {
  MockedResponder responder("code");
  EXPECT_CALL(*mView, addResponder("code")).Times(1);

  mController->addResponder(&responder);
}

TEST_F(ScreenSearchControllerFix, aNullResponderIsIgnored) {
  mController->addResponder(nullptr);
  mController->removeResponder(nullptr);
}

TEST_F(ScreenSearchControllerFix, searchOnlyReachesVisibleRespondersWhoseNameWasAsked) {
  StrictMock<MockedResponder> visible("code");
  StrictMock<MockedResponder> hidden("graph");
  EXPECT_CALL(*mView, addResponder(_)).Times(2);
  mController->addResponder(&visible);
  mController->addResponder(&hidden);

  // clearMatches() runs first, so every responder is cleared regardless of visibility.
  EXPECT_CALL(*mView, setMatchCount(0)).Times(1);
  EXPECT_CALL(visible, clearMatches()).Times(1);
  EXPECT_CALL(hidden, clearMatches()).Times(1);

  EXPECT_CALL(visible, isVisible()).WillOnce(Return(true));
  EXPECT_CALL(hidden, isVisible()).WillOnce(Return(false));
  EXPECT_CALL(visible, findMatches(mController, std::wstring(L"query"))).Times(1);

  mController->search(L"query", {"code"});
}

TEST_F(ScreenSearchControllerFix, anEmptyQuerySearchesNobodyButStillClears) {
  StrictMock<MockedResponder> responder("code");
  EXPECT_CALL(*mView, addResponder(_)).Times(1);
  mController->addResponder(&responder);

  EXPECT_CALL(*mView, setMatchCount(0)).Times(1);
  EXPECT_CALL(responder, clearMatches()).Times(1);
  EXPECT_CALL(responder, isVisible()).WillOnce(Return(true));

  mController->search(L"", {"code"});
}

TEST_F(ScreenSearchControllerFix, foundMatchesPublishesTheRunningTotal) {
  StrictMock<MockedResponder> responder("code");
  EXPECT_CALL(*mView, addResponder(_)).Times(1);
  mController->addResponder(&responder);

  EXPECT_CALL(*mView, setMatchCount(3)).Times(1);
  mController->foundMatches(&responder, 3);
}

TEST_F(ScreenSearchControllerFix, matchesFromAnUnknownResponderReturnEarlyWithoutTouchingTheView) {
  StrictMock<MockedResponder> stranger("nobody");

  // getResponderId() returns 0 for an unregistered responder and foundMatches() bails out before
  // it reaches setMatchCount() -- the strict view mock is what asserts that.
  mController->foundMatches(&stranger, 3);
}

TEST_F(ScreenSearchControllerFix, activateMatchWrapsForwardFromTheEndOfTheList) {
  StrictMock<MockedResponder> responder("code");
  EXPECT_CALL(*mView, addResponder(_)).Times(1);
  mController->addResponder(&responder);
  EXPECT_CALL(*mView, setMatchCount(2)).Times(1);
  mController->foundMatches(&responder, 2);

  // foundMatches leaves m_matchIndex one past the end, so the first "next" lands on index 0.
  EXPECT_CALL(responder, activateMatch(0)).Times(1);
  EXPECT_CALL(*mView, setMatchIndex(1)).Times(1);
  mController->activateMatch(true);

  EXPECT_CALL(responder, deactivateMatch(0)).Times(1);
  EXPECT_CALL(responder, activateMatch(1)).Times(1);
  EXPECT_CALL(*mView, setMatchIndex(2)).Times(1);
  mController->activateMatch(true);
}

TEST_F(ScreenSearchControllerFix, activateMatchDoesNothingWithoutMatches) {
  mController->activateMatch(true);
}

TEST_F(ScreenSearchControllerFix, everyWatchedMessageJustClearsTheMatches) {
  StrictMock<MockedResponder> responder("code");
  EXPECT_CALL(*mView, addResponder(_)).Times(1);
  mController->addResponder(&responder);

  EXPECT_CALL(*mView, setMatchCount(0)).Times(1);
  EXPECT_CALL(responder, clearMatches()).Times(1);

  MessageChangeFileView message(
      FilePath{}, MessageChangeFileView::FILE_SNIPPETS, MessageChangeFileView::VIEW_LIST, CodeScrollParams{});
  static_cast<MessageListener<MessageChangeFileView>*>(mController)->handleMessageBase(&message);
}
