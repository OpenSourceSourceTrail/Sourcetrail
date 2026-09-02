#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/ComponentFactory.h"
#include "component/controller/SearchController.h"
#include "component/view/SearchView.h"
#include "data/search/SearchMatch.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedSearchView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"
#include "type/search/MessageFind.h"
#include "type/search/MessageSearchAutocomplete.h"
#include "type/tab/MessageTabState.h"

using namespace testing;

namespace {

/**
 * MockedMessageQueue records dispatches instead of delivering them, so a controller's outbound
 * half is asserted by matching what reached pushMessage(). MATCHER_P keeps that readable.
 */
MATCHER_P(IsMessageOfType, type, "") {
  return arg != nullptr && arg->getType() == type;
}

SearchMatch namedMatch(const std::wstring& name) {
  SearchMatch match;
  match.name = name;
  match.text = name;
  return match;
}

}    // namespace

/**
 * Characterization tests for SearchController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct SearchControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<MockedSearchView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createSearchView(mViewLayout.get())).InSequence(mSequence).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createSearchComponent(mViewLayout.get());
    mController = mComponent->getController<SearchController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  testing::Sequence mSequence;
  std::shared_ptr<MockedSearchView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  SearchController* mController = nullptr;
};

TEST_F(SearchControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<SearchView>(), mView.get());
}

TEST_F(SearchControllerFix, schedulerIdIsTheTabId) {
  EXPECT_EQ(mController->getSchedulerId(), mComponent->getTabId());
}

TEST_F(SearchControllerFix, clearPushesEmptyMatchesToTheViewAndDispatchesTabState) {
  EXPECT_CALL(*mView, setMatches(IsEmpty())).Times(1);
  EXPECT_CALL(*mMessageQueue, pushMessage(IsMessageOfType("MessageTabState"))).Times(1);

  // clear() is a private override; reach it the way the framework does, through Controller.
  static_cast<Controller*>(mController)->clear();
}

TEST_F(SearchControllerFix, findFulltextIsForwardedToTheView) {
  EXPECT_CALL(*mView, findFulltext()).Times(1);

  MessageFind message(true);
  mController->MessageListener<MessageFind>::handleMessageBase(&message);
}

TEST_F(SearchControllerFix, plainFindOnlyFocusesTheView) {
  EXPECT_CALL(*mView, setFocus()).Times(1);

  MessageFind message(false);
  mController->MessageListener<MessageFind>::handleMessageBase(&message);
}

TEST_F(SearchControllerFix, autocompleteQueriesStorageWhenTheQueryIsStillCurrent) {
  const std::wstring query = L"foo";
  const std::vector<SearchMatch> matches{namedMatch(L"foobar")};

  EXPECT_CALL(*mView, getQuery()).WillOnce(Return(query));
  EXPECT_CALL(*mStorageAccess, getAutocompletionMatches(query, _, true)).WillOnce(Return(matches));
  EXPECT_CALL(*mView, setAutocompletionList(SizeIs(1))).Times(1);

  MessageSearchAutocomplete message(query, NodeTypeSet::all());
  mController->MessageListener<MessageSearchAutocomplete>::handleMessageBase(&message);
}

TEST_F(SearchControllerFix, staleAutocompleteIsDroppedWithoutTouchingStorage) {
  // The view has moved on to a different query, so the in-flight request is abandoned. Both mocks
  // are strict, so any call to storage or setAutocompletionList would fail this test.
  EXPECT_CALL(*mView, getQuery()).WillOnce(Return(std::wstring(L"newer")));

  MessageSearchAutocomplete message(L"stale", NodeTypeSet::all());
  mController->MessageListener<MessageSearchAutocomplete>::handleMessageBase(&message);
}
