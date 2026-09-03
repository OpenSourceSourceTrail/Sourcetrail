#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ide_communication/logic/IDECommunicationController.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedStorageAccess.hpp"

using namespace testing;

namespace {

/**
 * IDECommunicationController is abstract -- the transport (startListening/sendMessage) is supplied
 * by the Qt subclass. This stands in for it and records what was put on the wire.
 */
struct TestableIDECommunicationController final : IDECommunicationController {
  explicit TestableIDECommunicationController(StorageAccess* storageAccess) : IDECommunicationController(storageAccess) {}

  void startListening() override {
    ++mStartCount;
  }
  void stopListening() override {
    ++mStopCount;
  }
  [[nodiscard]] bool isListening() const override {
    return true;
  }

  void sendMessage(const std::wstring& message) const override {
    mSent.push_back(message);
  }

  int mStartCount = 0;
  int mStopCount = 0;
  mutable std::vector<std::wstring> mSent;
};

}    // namespace

/**
 * Characterization tests for IDECommunicationController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct IDECommunicationControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, pushMessage(_)).Times(AnyNumber()).WillRepeatedly([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    mController = std::make_unique<TestableIDECommunicationController>(mStorageAccess.get());
  }

  void TearDown() override {
    mController.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::vector<std::string> mDispatched;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  std::unique_ptr<TestableIDECommunicationController> mController;
};

TEST_F(IDECommunicationControllerFix, listeningStartsEnabled) {
  EXPECT_TRUE(mController->getEnabled());
}

TEST_F(IDECommunicationControllerFix, clearIsANoOp) {
  mController->clear();
  EXPECT_TRUE(mController->getEnabled());
}

TEST_F(IDECommunicationControllerFix, incomingMessagesAreIgnoredWhileDisabled) {
  mController->setEnabled(false);

  // The storage mock is strict, so any parse that reached storage would fail this test.
  mController->handleIncomingMessage(L"setActiveToken>>foo>>1>>1>>");

  EXPECT_THAT(mDispatched, IsEmpty());
}

TEST_F(IDECommunicationControllerFix, anUnparseableMessageIsLoggedAndDropped) {
  mController->handleIncomingMessage(L"not a protocol message at all");

  EXPECT_THAT(mDispatched, IsEmpty());
}

TEST_F(IDECommunicationControllerFix, aValidPingDispatchesPingReceived) {
  mController->handleIncomingMessage(NetworkProtocolHelper::buildPingMessage());

  EXPECT_THAT(mDispatched, Contains("MessagePingReceived"));
}

TEST_F(IDECommunicationControllerFix, windowFocusInSendsAPingAndResetsTheConnectionStatus) {
  MessageWindowFocus message(true);
  static_cast<MessageListener<MessageWindowFocus>*>(mController.get())->handleMessageBase(&message);

  EXPECT_THAT(mDispatched, Contains("MessagePingReceived"));
  EXPECT_THAT(mController->mSent, ElementsAre(NetworkProtocolHelper::buildPingMessage()));
}

TEST_F(IDECommunicationControllerFix, windowFocusOutSendsNothing) {
  MessageWindowFocus message(false);
  static_cast<MessageListener<MessageWindowFocus>*>(mController.get())->handleMessageBase(&message);

  EXPECT_THAT(mController->mSent, IsEmpty());
}

TEST_F(IDECommunicationControllerFix, createCdbRequestGoesOutOnTheWireWithAStatusMessage) {
  MessageIDECreateCDB message;
  static_cast<MessageListener<MessageIDECreateCDB>*>(mController.get())->handleMessageBase(&message);

  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
  EXPECT_THAT(mController->mSent, ElementsAre(NetworkProtocolHelper::buildCreateCDBMessage()));
}

TEST_F(IDECommunicationControllerFix, moveCursorGoesOutOnTheWireWithAStatusMessage) {
  MessageMoveIDECursor message(FilePath(L"/tmp/foo.cpp"), 12, 3);
  static_cast<MessageListener<MessageMoveIDECursor>*>(mController.get())->handleMessageBase(&message);

  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
  EXPECT_THAT(mController->mSent, ElementsAre(NetworkProtocolHelper::buildSetIDECursorMessage(FilePath(L"/tmp/foo.cpp"), 12, 3)));
}

TEST_F(IDECommunicationControllerFix, aPortChangeRestartsTheListener) {
  MessagePluginPortChange message;
  static_cast<MessageListener<MessagePluginPortChange>*>(mController.get())->handleMessageBase(&message);

  EXPECT_EQ(mController->mStopCount, 1);
  EXPECT_EQ(mController->mStartCount, 1);
}
