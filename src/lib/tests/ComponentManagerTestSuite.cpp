#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ComponentManager.h"
#include "mocks/MockedBookmarkButtonsView.hpp"
#include "mocks/MockedCodeView.hpp"
#include "mocks/MockedDialogView.hpp"
#include "mocks/MockedGraphView.hpp"
#include "mocks/MockedScreenSearchSender.hpp"
#include "mocks/MockedScreenSearchView.hpp"
#include "mocks/MockedSearchView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedUndoRedoView.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"

using namespace testing;

struct SetupMainFix : Test {
  void SetUp() override {
    EXPECT_CALL(factory, createCompositeView(_, _, _, _)).WillOnce(Return(nullptr));

    EXPECT_CALL(mockedLayout, setViewEnabled(_, _)).Times(5);

    auto undoRedoView = std::make_shared<MockedUndoRedoView>(&mockedLayout);
    EXPECT_CALL(factory, createUndoRedoView(_)).WillOnce(Return(undoRedoView));
    EXPECT_CALL(factory, createRefreshView(_)).WillOnce(Return(nullptr));
    auto searchView = std::make_shared<MockedSearchView>(&mockedLayout);
    EXPECT_CALL(factory, createSearchView(_)).WillOnce(Return(searchView));

    auto bookmarkButtonsView = std::make_shared<MockedBookmarkButtonsView>(&mockedLayout);
    EXPECT_CALL(factory, createBookmarkButtonsView(_)).WillOnce(Return(bookmarkButtonsView));

    auto graphView = std::make_shared<MockedGraphView>(&mockedLayout);
    EXPECT_CALL(factory, createGraphView(_)).WillOnce(Return(graphView));
    auto codeView = std::make_shared<MockedCodeView>(&mockedLayout);
    EXPECT_CALL(factory, createCodeView(_)).WillOnce(Return(codeView));

    auto mockedScreenSearchView = std::make_shared<MockedScreenSearchView>(&mockedLayout);
    EXPECT_CALL(*mockedScreenSearchView, addResponder(_)).Times(2);
    EXPECT_CALL(factory, createScreenSearchView(_)).WillOnce(Return(mockedScreenSearchView));

    auto dialogView = std::make_shared<MockedDialogView>();
    EXPECT_CALL(factory, createDialogView(_, _, _)).Times(3).WillRepeatedly(Return(dialogView));

    EXPECT_CALL(factory, createTabbedView(_, _)).WillOnce(Return(nullptr));

    EXPECT_CALL(factory, createTabsView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createTooltipView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createStatusView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createErrorView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createBookmarkView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createStatusBarView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createCustomTrailView(_)).WillOnce(Return(nullptr));

    manager = std::make_unique<ComponentManager>(&factory, &mockedStorageAccess);
  }

  StrictMock<MockedViewFactory> factory;
  MockedViewLayout mockedLayout;
  MockedStorageAccess mockedStorageAccess;
  std::unique_ptr<ComponentManager> manager;
};

TEST_F(SetupMainFix, goodCase) {
  manager->setupMain(&mockedLayout, 0);
  auto view = manager->getDialogView(DialogView::UseCase::INDEXING);
  ASSERT_TRUE(view);
  manager->clear();
  manager->clearComponents();
}

struct SetupTabFix : Test {
  void SetUp() override {
    EXPECT_CALL(factory, createCompositeView(_, _, _, _)).WillOnce(Return(nullptr));

    EXPECT_CALL(factory, createUndoRedoView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createRefreshView(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(factory, createSearchView(_)).WillOnce(Return(nullptr));

    auto bookmarkButtonsView = std::make_shared<MockedBookmarkButtonsView>(&mockedLayout);
    EXPECT_CALL(factory, createBookmarkButtonsView(_)).WillOnce(Return(bookmarkButtonsView));

    auto graphView = std::make_shared<MockedGraphView>(&mockedLayout);
    EXPECT_CALL(factory, createGraphView(_)).WillOnce(Return(graphView));
    auto codeView = std::make_shared<MockedCodeView>(&mockedLayout);
    EXPECT_CALL(factory, createCodeView(_)).WillOnce(Return(codeView));

    EXPECT_CALL(sender, addResponder(_)).Times(2);
  }

  MockedScreenSearchSender sender;
  StrictMock<MockedViewFactory> factory;
  MockedViewLayout mockedLayout;
  std::unique_ptr<ComponentManager> manager = std::make_unique<ComponentManager>(&factory, nullptr);
};

TEST_F(SetupTabFix, goodCase) {
  manager->setupTab(&mockedLayout, Id {0}, &sender);
}