#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "component/controller/ErrorController.h"
#include "component/view/ErrorView.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedErrorView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"

using namespace testing;

namespace {

ErrorInfo anError(Id id, const std::wstring& message) {
  ErrorInfo info;
  info.id = id;
  info.message = message;
  info.fatal = false;
  info.indexed = true;
  return info;
}

}    // namespace

/**
 * Characterization tests for ErrorController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 *
 * Two handlers are deliberately not covered: MessageErrorCountUpdate and MessageErrorsForFile both
 * call Application::getInstance(), so they cannot run without a live Application. That reach-through
 * is one of the leaks the restructuring is meant to close -- see Phase 4 of the plan.
 */
struct ErrorControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, pushMessage(_)).Times(AnyNumber()).WillRepeatedly([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedErrorView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createErrorView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createErrorComponent(mViewLayout.get());
    mController = mComponent->getController<ErrorController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::vector<std::string> mDispatched;
  std::shared_ptr<MockedErrorView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  ErrorController* mController = nullptr;
};

TEST_F(ErrorControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<ErrorView>(), mView.get());
}

TEST_F(ErrorControllerFix, clearOnlyClearsTheView) {
  EXPECT_CALL(*mView, clear()).Times(1);

  static_cast<Controller*>(mController)->clear();
}

TEST_F(ErrorControllerFix, activatingErrorsClearsThenRefillsTheView) {
  const ErrorFilter filter;

  EXPECT_CALL(*mView, clear()).Times(1);
  EXPECT_CALL(*mView, setErrorFilter(_)).Times(1);
  // No active file path for the tab, so the unfiltered-by-file query is the one that runs.
  EXPECT_CALL(*mStorageAccess, getErrorsLimited(_)).WillOnce(Return(std::vector<ErrorInfo>{anError(1, L"boom")}));
  EXPECT_CALL(*mView, addErrors(SizeIs(1), _, true)).Times(1);
  // showErrors() returned true, so the dock is raised -- View::showDockWidget() routes to the layout.
  EXPECT_CALL(*mViewLayout, showView(mView.get())).Times(1);

  MessageActivateErrors message(filter);
  static_cast<MessageListener<MessageActivateErrors>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, activatingErrorsLeavesTheDockAloneWhenThereAreNone) {
  EXPECT_CALL(*mView, clear()).Times(1);
  EXPECT_CALL(*mView, setErrorFilter(_)).Times(1);
  EXPECT_CALL(*mStorageAccess, getErrorsLimited(_)).WillOnce(Return(std::vector<ErrorInfo>{}));
  EXPECT_CALL(*mView, addErrors(IsEmpty(), _, true)).Times(1);

  MessageActivateErrors message{ErrorFilter()};
  static_cast<MessageListener<MessageActivateErrors>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, clearingTheCountResetsTheFilterLimitToItsDefault) {
  ErrorFilter narrowed;
  narrowed.limit = 3;

  EXPECT_CALL(*mView, clear()).Times(1);
  EXPECT_CALL(*mView, getErrorFilter()).WillOnce(Return(narrowed));
  EXPECT_CALL(*mView, setErrorFilter(Field(&ErrorFilter::limit, ErrorFilter().limit))).Times(1);

  MessageErrorCountClear message;
  static_cast<MessageListener<MessageErrorCountClear>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, indexingStartedTurnsTheErrorCacheOn) {
  EXPECT_CALL(*mStorageAccess, setUseErrorCache(true)).Times(1);

  MessageIndexingStarted message;
  static_cast<MessageListener<MessageIndexingStarted>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, indexingFinishedTurnsTheCacheOffAndRedrawsWithoutScrolling) {
  EXPECT_CALL(*mStorageAccess, setUseErrorCache(false)).Times(1);
  EXPECT_CALL(*mView, clear()).Times(1);
  EXPECT_CALL(*mView, getErrorFilter()).WillOnce(Return(ErrorFilter()));
  EXPECT_CALL(*mStorageAccess, getErrorsLimited(_)).WillOnce(Return(std::vector<ErrorInfo>{}));
  EXPECT_CALL(*mView, addErrors(IsEmpty(), _, false)).Times(1);

  MessageIndexingFinished message;
  static_cast<MessageListener<MessageIndexingFinished>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, showErrorMessageOnlyForwardsTheIdToTheView) {
  EXPECT_CALL(*mView, setErrorId(42)).Times(1);

  MessageShowError message(42);
  static_cast<MessageListener<MessageShowError>*>(mController)->handleMessageBase(&message);
}

TEST_F(ErrorControllerFix, errorsAllRedispatchesTheViewsCurrentFilter) {
  EXPECT_CALL(*mView, getErrorFilter()).WillOnce(Return(ErrorFilter()));

  MessageErrorsAll message;
  static_cast<MessageListener<MessageErrorsAll>*>(mController)->handleMessageBase(&message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateErrors"));
}

TEST_F(ErrorControllerFix, errorFilterChangedDispatchesActivateErrors) {
  mController->errorFilterChanged(ErrorFilter());

  EXPECT_THAT(mDispatched, Contains("MessageActivateErrors"));
}

TEST_F(ErrorControllerFix, showErrorRefreshesTheFilterFirstBecauseTheTabIsNotShowingErrorsYet) {
  EXPECT_CALL(*mView, getErrorFilter()).WillOnce(Return(ErrorFilter()));

  mController->showError(7);

  EXPECT_THAT(mDispatched, Contains("MessageActivateErrors"));
  EXPECT_THAT(mDispatched, Contains("MessageShowError"));
}
