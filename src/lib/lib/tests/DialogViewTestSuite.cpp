#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/view/DialogView.h"
#include "MessageQueue.h"
#include "MockedMessageQueue.hpp"
#include "type/indexing/MessageIndexingStatus.h"

using namespace testing;

namespace {

// The counts the dialog draws and the percentage the status bar shows come from the same call, so
// this covers both. DialogView's own doUpdateIndexingDialog is a no-op, which is all we need.
struct DialogViewFix : Test {
  void SetUp() override {
    messageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(messageQueue);
  }

  void TearDown() override {
    IMessageQueue::setInstance(nullptr);
  }

  // Returns the percentage of the single MessageIndexingStatus the call published.
  size_t publishedPercent(size_t finishedFileCount, size_t totalFileCount) {
    std::shared_ptr<MessageBase> published;
    EXPECT_CALL(*messageQueue, pushMessage).WillOnce(SaveArg<0>(&published));

    DialogView view(DialogView::UseCase::INDEXING, nullptr);
    view.updateIndexingDialog(finishedFileCount, finishedFileCount, totalFileCount, {});

    auto* status = dynamic_cast<MessageIndexingStatus*>(published.get());
    EXPECT_NE(nullptr, status);
    EXPECT_TRUE(status->showProgress);
    return status != nullptr ? status->progressPercent : 0;
  }

  std::shared_ptr<MockedMessageQueue> messageQueue;
};

TEST_F(DialogViewFix, publishesProgressPercent) {
  EXPECT_EQ(50, publishedPercent(5, 10));
}

TEST_F(DialogViewFix, publishesZeroBeforeAnyFileIsCounted) {
  EXPECT_EQ(0, publishedPercent(0, 10));
}

// Nothing is known about the work yet on the first update, and 0/0 must not divide by zero.
TEST_F(DialogViewFix, publishesZeroWhenTheTotalIsUnknown) {
  EXPECT_EQ(0, publishedPercent(0, 0));
}

TEST_F(DialogViewFix, publishesHundredWhenEveryFileIsDone) {
  EXPECT_EQ(100, publishedPercent(10, 10));
}

}    // namespace
