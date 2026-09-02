#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "activation/logic/ActivationController.h"
#include "data/graph/Edge.h"
#include "data/name/NameHierarchy.h"
#include "FilePath.h"
#include "MockedMessageQueue.hpp"
#include "mocks/MockedApplicationSetting.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "search/domain/SearchMatch.h"

using namespace testing;

/**
 * Characterization tests for ActivationController.
 *
 * The suite this replaces was never registered in CMake, so it had not compiled in a long time --
 * it included a ConsoleLogger.h that no longer exists, and drove a real threaded MessageQueue with
 * sleeps. This one follows the shape the other controller suites use: MockedMessageQueue records
 * what was dispatched rather than delivering it, so the assertions are on the message traffic the
 * controller produces.
 *
 * They pin what the controller does today; they are not a specification of what it should do.
 */
struct ActivationControllerFix : Test {
  void SetUp() override {
    mMessageQueue = std::make_shared<MockedMessageQueue>();
    IMessageQueue::setInstance(mMessageQueue);

    EXPECT_CALL(*mMessageQueue, registerListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, unregisterListener(_)).Times(AnyNumber());
    EXPECT_CALL(*mMessageQueue, pushMessage(_)).Times(AnyNumber()).WillRepeatedly([this](std::shared_ptr<MessageBase> message) {
      mDispatched.push_back(message->getType());
    });
    EXPECT_CALL(*mMessageQueue, processMessage(_, _))
        .Times(AnyNumber())
        .WillRepeatedly([this](const std::shared_ptr<MessageBase>& message, bool) { mDispatched.push_back(message->getType()); });

    mAppSettings = std::make_shared<NiceMock<MockedApplicationSettings>>();
    IApplicationSettings::setInstance(mAppSettings);

    mController = std::make_unique<ActivationController>(&mStorageAccess);
  }

  void TearDown() override {
    mController.reset();
    IApplicationSettings::setInstance(nullptr);
    mAppSettings.reset();
    IMessageQueue::setInstance(nullptr);
    mMessageQueue.reset();
  }

  template <typename MessageType>
  void deliver(MessageType& message) {
    static_cast<MessageListener<MessageType>*>(mController.get())->handleMessageBase(&message);
  }

  std::shared_ptr<MockedMessageQueue> mMessageQueue;
  std::shared_ptr<NiceMock<MockedApplicationSettings>> mAppSettings;
  std::vector<std::string> mDispatched;
  StrictMock<MockedStorageAccess> mStorageAccess;
  std::unique_ptr<ActivationController> mController;
};

TEST_F(ActivationControllerFix, clearIsANoOp) {
  mController->clear();
  EXPECT_THAT(mDispatched, IsEmpty());
}

TEST_F(ActivationControllerFix, activatingAnEdgeActivatesItsOwnTokenId) {
  MessageActivateEdge message(1, Edge::EDGE_CALL, NameHierarchy{}, NameHierarchy{});
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}

TEST_F(ActivationControllerFix, activatingABundledEdgeActivatesTheBundleInstead) {
  MessageActivateEdge message(1, Edge::EDGE_BUNDLED_EDGES, NameHierarchy{}, NameHierarchy{});
  message.bundledEdgesIds = {2, 3};
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}

TEST_F(ActivationControllerFix, activatingAKnownFileActivatesItsNode) {
  EXPECT_CALL(mStorageAccess, getNodeIdForFileNode(_)).WillOnce(Return(7));
  EXPECT_CALL(mStorageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch>{}));

  MessageActivateFile message{FilePath{L"/tmp/foo.cpp"}};
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}

TEST_F(ActivationControllerFix, activatingAnUnknownFileFallsBackToChangingTheFileView) {
  EXPECT_CALL(mStorageAccess, getNodeIdForFileNode(_)).WillOnce(Return(0));

  MessageActivateFile message{FilePath{L"/tmp/foo.cpp"}};
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageChangeFileView"));
  EXPECT_THAT(mDispatched, Not(Contains("MessageActivateTokens")));
}

TEST_F(ActivationControllerFix, activatingAFileWithALineAlsoScrolls) {
  EXPECT_CALL(mStorageAccess, getNodeIdForFileNode(_)).WillOnce(Return(0));

  MessageActivateFile message{FilePath{L"/tmp/foo.cpp"}, 42};
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageScrollToLine"));
}

TEST_F(ActivationControllerFix, activatingNodesResolvesTheOnesGivenOnlyByName) {
  EXPECT_CALL(mStorageAccess, getNodeIdForNameHierarchy(_)).WillOnce(Return(5));
  EXPECT_CALL(mStorageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch>{}));

  MessageActivateNodes message;
  message.addNode(0, NameHierarchy{L"foo", NAME_DELIMITER_CXX});
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}

TEST_F(ActivationControllerFix, activatingTokenIdsLooksUpTheirSearchMatches) {
  EXPECT_CALL(mStorageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch>{}));

  MessageActivateTokenIds message({1, 2});
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}

TEST_F(ActivationControllerFix, activatingSourceLocationsActivatesTheirNodes) {
  EXPECT_CALL(mStorageAccess, getNodeIdsForLocationIds(_)).WillOnce(Return(std::vector<Id>{9}));

  MessageActivateSourceLocations message({1}, false);
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateNodes"));
}

TEST_F(ActivationControllerFix, resetZoomOnlyRefreshesWhenTheFontSizeActuallyChanges) {
  EXPECT_CALL(*mAppSettings, getFontSizeStd()).WillRepeatedly(Return(12));
  EXPECT_CALL(*mAppSettings, getFontSize()).WillRepeatedly(Return(12));

  MessageResetZoom message;
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
  EXPECT_THAT(mDispatched, Not(Contains("MessageRefreshUI")));
}

TEST_F(ActivationControllerFix, resetZoomRefreshesTheUiWhenItHasToRestoreTheSize) {
  EXPECT_CALL(*mAppSettings, getFontSizeStd()).WillRepeatedly(Return(12));
  EXPECT_CALL(*mAppSettings, getFontSize()).WillRepeatedly(Return(20));
  EXPECT_CALL(*mAppSettings, setFontSize(12));

  MessageResetZoom message;
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageRefreshUI"));
  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
}

TEST_F(ActivationControllerFix, zoomingInAtTheMaximumIsIgnored) {
  EXPECT_CALL(*mAppSettings, getFontSize()).WillRepeatedly(Return(24));
  EXPECT_CALL(*mAppSettings, getFontSizeMax()).WillRepeatedly(Return(24));
  EXPECT_CALL(*mAppSettings, getFontSizeMin()).WillRepeatedly(Return(6));

  MessageZoom message(true);
  deliver(message);

  EXPECT_THAT(mDispatched, IsEmpty());
}

TEST_F(ActivationControllerFix, zoomingInBelowTheMaximumStepsTheFontSizeUp) {
  EXPECT_CALL(*mAppSettings, getFontSize()).WillRepeatedly(Return(12));
  EXPECT_CALL(*mAppSettings, getFontSizeMax()).WillRepeatedly(Return(24));
  EXPECT_CALL(*mAppSettings, getFontSizeMin()).WillRepeatedly(Return(6));
  EXPECT_CALL(*mAppSettings, setFontSize(13));

  MessageZoom message(true);
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageStatus"));
  EXPECT_THAT(mDispatched, Contains("MessageRefreshUI"));
}

TEST_F(ActivationControllerFix, aSearchForTheErrorCommandGoesToTheErrorFeature) {
  SearchMatch match;
  match.searchType = SearchMatch::SEARCH_COMMAND;
  match.name = SearchMatch::getCommandName(SearchMatch::COMMAND_ERROR);

  MessageSearch message({match}, NodeTypeSet::all());
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageErrorsAll"));
}

TEST_F(ActivationControllerFix, aPlainSearchActivatesTheMatchedTokens) {
  SearchMatch match;
  match.searchType = SearchMatch::SEARCH_TOKEN;
  match.tokenIds = {3};

  MessageSearch message({match}, NodeTypeSet::all());
  deliver(message);

  EXPECT_THAT(mDispatched, Contains("MessageActivateTokens"));
}
