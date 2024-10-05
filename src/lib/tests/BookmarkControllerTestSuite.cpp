#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "BookmarkController.h"
#include "BookmarkView.h"
#include "ComponentFactory.h"
#include "type/bookmark/MessageBookmarkUpdate.hpp"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedBookmarkView.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"

using namespace testing;
using testing::Return;

struct MockedMessageBookmarkUpdate : public MessageListener<MessageBookmarkUpdate> {
  uint32_t mCountOfCall = 0;
  void handleMessage(MessageBookmarkUpdate*) override {
    ++mCountOfCall;
  }
};

struct BookmarkControllerFix : public Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    mBookmarkUpdate = std::make_unique<MockedMessageBookmarkUpdate>();

    mViewLayout = std::make_unique<StrictMock<MockedViewLayout>>();
    mView = std::make_shared<StrictMock<MockedBookmarkView>>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createBookmarkView(mViewLayout.get())).InSequence(mSequence).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createBookmarkComponent(mViewLayout.get());
    mController = mComponent->getController<BookmarkController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mBookmarkUpdate.reset();
    mComponent.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  void MockUpdate(bool skipNodes = false, bool skipEdges = false) {
    EXPECT_CALL(*mView, bookmarkBrowserIsVisible()).InSequence(mSequence).WillOnce(Return(false));
    if(!skipNodes) {
      EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
    }
    if(!skipEdges) {
      EXPECT_CALL(*mStorageAccess, getAllEdgeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::EdgeBookmarks {}));
    }
  }

  void MockCleanBookmarkCategories() {
    EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
    EXPECT_CALL(*mStorageAccess, getAllEdgeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::EdgeBookmarks {}));
    EXPECT_CALL(*mStorageAccess, getAllBookmarkCategories())
        .InSequence(mSequence)
        .WillOnce(Return(MockedStorageAccess::BookmarkCategories {}));
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  testing::Sequence mSequence;
  std::unique_ptr<MockedMessageBookmarkUpdate> mBookmarkUpdate;
  std::shared_ptr<StrictMock<MockedBookmarkView>> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<StrictMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  BookmarkController* mController = nullptr;
};

TEST_F(BookmarkControllerFix, clear) {
  mController->clear();
}

TEST_F(BookmarkControllerFix, DISABLED_displayBookmarks) {
  EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
  EXPECT_CALL(*mStorageAccess, getAllEdgeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::EdgeBookmarks {}));
  EXPECT_CALL(*mView, displayBookmarks(_)).InSequence(mSequence).WillOnce(Return());
  mController->displayBookmarks();
}

TEST_F(BookmarkControllerFix, DISABLED_displayBookmarksFor) {
  EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
  EXPECT_CALL(*mStorageAccess, getAllEdgeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::EdgeBookmarks {}));
  EXPECT_CALL(*mView, displayBookmarks(_)).InSequence(mSequence).WillOnce(Return());
  mController->displayBookmarksFor(Bookmark::Filter::Unknown, Bookmark::Order::None);
}

TEST_F(BookmarkControllerFix, displayBookmarksFor2) {
  EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
  EXPECT_CALL(*mStorageAccess, getAllEdgeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::EdgeBookmarks {}));
  EXPECT_CALL(*mView, displayBookmarks(_)).InSequence(mSequence).WillOnce(Return());
  mController->displayBookmarksFor(Bookmark::Filter::All, Bookmark::Order::DateDescending);
}

TEST_F(BookmarkControllerFix, DISABLED_createBookmark) {
  EXPECT_CALL(*mStorageAccess, addNodeBookmark(_)).InSequence(mSequence).WillOnce(Return(0));
  MockUpdate();

  const std::wstring name = L"bookmark";
  const std::wstring comment = L"comment";
  const std::wstring category = L"category";
  constexpr Id id = 10;
  mController->createBookmark(name, comment, category, id);

  EXPECT_EQ(1, mBookmarkUpdate->mCountOfCall);
}

TEST_F(BookmarkControllerFix, DISABLED_createBookmarkWithDefaultId) {
  EXPECT_CALL(*mStorageAccess, addNodeBookmark(_)).InSequence(mSequence).WillOnce(Return(0));
  MockUpdate();

  const std::wstring name = L"bookmark";
  const std::wstring comment = L"comment";
  const std::wstring category = L"category";
  constexpr Id id = 0;
  mController->createBookmark(name, comment, category, id);

  EXPECT_EQ(1, mBookmarkUpdate->mCountOfCall);
}

TEST_F(BookmarkControllerFix, editBookmark) {
  EXPECT_CALL(*mStorageAccess, updateBookmark(_, _, _, _)).InSequence(mSequence).WillOnce(Return());
  MockCleanBookmarkCategories();
  MockUpdate(true, true);

  const std::wstring name = L"bookmark";
  const std::wstring comment = L"comment";
  const std::wstring category = L"category";
  constexpr Id id = 10;
  mController->editBookmark(id, name, comment, category);
}

TEST_F(BookmarkControllerFix, deleteBookmark) {
  EXPECT_CALL(*mStorageAccess, removeBookmark(_)).InSequence(mSequence).WillOnce(Return());
  MockCleanBookmarkCategories();
  MockUpdate(true, true);

  constexpr Id id = 10;
  mController->deleteBookmark(id);
}

TEST_F(BookmarkControllerFix, deleteCategory) {
  EXPECT_CALL(*mStorageAccess, removeBookmarkCategory(_)).InSequence(mSequence).WillOnce(Return());
  EXPECT_CALL(*mStorageAccess, getAllNodeBookmarks()).InSequence(mSequence).WillOnce(Return(MockedStorageAccess::NodeBookmarks {}));
  MockUpdate(true);

  constexpr Id id = 10;
  mController->deleteBookmarkCategory(id);
}