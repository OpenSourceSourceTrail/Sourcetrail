#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "type/focus/MessageFocusView.h"
#include "type/MessageRefreshUI.h"
#include "MockedMessageQueue.hpp"
#include "Tab.h"
#include "mocks/MockedBookmarkButtonsView.hpp"
#include "mocks/MockedCodeView.hpp"
#include "mocks/MockedCompositeView.hpp"
#include "mocks/MockedGraphView.hpp"
#include "mocks/MockedRefreshView.hpp"
#include "mocks/MockedScreenSearchSender.hpp"
#include "mocks/MockedSearchView.hpp"
#include "mocks/MockedUndoRedoView.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"

using namespace testing;

struct TabIdFix : Test {
  void SetUp() override {
    mockSetupTab();
    mockRefreshViews();
    mockTeardownTab();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  void mockSetupTab() {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);
    mockedCompositeView = std::make_shared<MockedCompositeView>(&mockedLayout);
    undoRedoView = std::make_shared<MockedUndoRedoView>(&mockedLayout);
    refreshView = std::make_shared<MockedRefreshView>(&mockedLayout);
    searchView = std::make_shared<MockedSearchView>(&mockedLayout);
    bookmarkButtonsView = std::make_shared<MockedBookmarkButtonsView>(&mockedLayout);
    graphView = std::make_shared<MockedGraphView>(&mockedLayout);
    codeView = std::make_shared<MockedCodeView>(&mockedLayout);
    EXPECT_CALL(viewFactory, createCompositeView(_, _, _, _)).WillOnce(Return(mockedCompositeView));
    EXPECT_CALL(viewFactory, createUndoRedoView(_)).WillOnce(Return(undoRedoView));
    EXPECT_CALL(viewFactory, createRefreshView(_)).WillOnce(Return(refreshView));
    EXPECT_CALL(viewFactory, createSearchView(_)).WillOnce(Return(searchView));
    EXPECT_CALL(viewFactory, createBookmarkButtonsView(_)).WillOnce(Return(bookmarkButtonsView));

    EXPECT_CALL(screenSearchSender, addResponder(_)).Times(2);
    EXPECT_CALL(viewFactory, createGraphView(_)).WillOnce(Return(graphView));
    EXPECT_CALL(viewFactory, createCodeView(_)).WillOnce(Return(codeView));
  }

  void mockRefreshViews() const {
    EXPECT_CALL(*undoRedoView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*refreshView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*searchView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*graphView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*codeView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*mockedCompositeView, refreshView()).WillOnce(Return());
    EXPECT_CALL(*bookmarkButtonsView, refreshView()).WillOnce(Return());
  }

  void mockTeardownTab() {
    EXPECT_CALL(screenSearchSender, removeResponder(_)).Times(10);
  }

  void TearDown() override {
    mockedCompositeView.reset();
    bookmarkButtonsView.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedCompositeView> mockedCompositeView;
  std::shared_ptr<MockedUndoRedoView> undoRedoView;
  std::shared_ptr<MockedRefreshView> refreshView;
  std::shared_ptr<MockedSearchView> searchView;
  std::shared_ptr<MockedBookmarkButtonsView> bookmarkButtonsView;
  std::shared_ptr<MockedGraphView> graphView;
  std::shared_ptr<MockedCodeView> codeView;

  StrictMock<MockedViewFactory> viewFactory;
  MockedScreenSearchSender screenSearchSender;
  MockedViewLayout mockedLayout;

  std::thread thread;
};

// NOLINTNEXTLINE
TEST_F(TabIdFix, DISABLED_goodCase) {
  Tab tab(0, &viewFactory, nullptr, &screenSearchSender);

  EXPECT_EQ(0, tab.getSchedulerId());

  tab.setParentLayout(&mockedLayout);

  EXPECT_CALL(*graphView, setNavigationFocus(_)).WillOnce(Return());
  EXPECT_CALL(*codeView, setNavigationFocus(_)).WillOnce(Return());
  MessageFocusView {MessageFocusView::ViewType::CODE}.dispatchImmediately();

  mockRefreshViews();
  MessageRefreshUI {}.dispatchImmediately();
}

// NOLINTNEXTLINE
TEST_F(TabIdFix, DISABLED_editView) {
  Tab tab(0, &viewFactory, nullptr, &screenSearchSender);
  MockedCompositeView view(&mockedLayout);
  tab.addView(&view);
  tab.showView(&view);
  tab.hideView(&view);
  tab.removeView(&view);
}