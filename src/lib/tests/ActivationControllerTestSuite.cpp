#include <iostream>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ActivationController.h"
#include "ConsoleLogger.h"
#include "MessageActivateEdge.h"
#include "MessageActivateTokenIds.h"
#include "MessageActivateTokens.h"
#include "MessageChangeFileView.h"
#include "MessageRefreshUI.h"
#include "MessageScrollToLine.h"
#include "MessageSearch.h"
#include "MessageStatus.h"
#include "NameHierarchy.h"
#include "IApplicationSettings.hpp"
#include "mocks/MockedStorageAccess.hpp"

using namespace std::chrono_literals;
using namespace testing;

namespace {
void addConsoleLogger() {
  auto consoleLogger = std::make_shared<ConsoleLogger>();
  consoleLogger->setLogLevel(Logger::LogLevel::LOG_ALL);
  LogManager::getInstance()->setLoggingEnabled(true);
  LogManager::getInstance()->addLogger(consoleLogger);
}
}    // namespace

struct ActivationControllerFix : Test {
  static void SetUpTestSuite() {
    addConsoleLogger();
    MessageQueue::getInstance()->startMessageLoopThreaded();
  }

  static void TearDownTestSuite() {
    MessageQueue::getInstance()->stopMessageLoop();
  }

  MockedStorageAccess storageAccess;
};

struct SpyMessageActivateTokens : MessageListener<MessageActivateTokens> {
  bool mCalled = false;
  void handleMessage(MessageActivateTokens* /*message*/) override {
    mCalled = true;
  }
};

TEST_F(ActivationControllerFix, MessageActivateEdge) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateTokens spyMessageActivateTokens;
  MessageActivateEdge {
      Id {0},
      Edge::EdgeType::EDGE_CALL,
      NameHierarchy {},
      NameHierarchy {},
  }
      .dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

TEST_F(ActivationControllerFix, MessageActivateBundleEdge) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateTokens spyMessageActivateTokens;
  MessageActivateEdge {
      Id {0},
      Edge::EdgeType::EDGE_BUNDLED_EDGES,    // isBundledEdges
      NameHierarchy {},
      NameHierarchy {},
  }
      .dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

TEST_F(ActivationControllerFix, MessageActivateTokens) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateTokens spyMessageActivateTokens;
  EXPECT_CALL(storageAccess, getNodeIdForFileNode(_)).WillOnce(Return(1));
  EXPECT_CALL(storageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch> {}));
  MessageActivateFile {FilePath {}}.dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

struct SpyMessageChangeFileView
    : MessageListener<MessageChangeFileView>
    , MessageListener<MessageScrollToLine> {
  bool mMessageChangeFileViewCalled = false;
  void handleMessage(MessageChangeFileView* /*message*/) override {
    mMessageChangeFileViewCalled = true;
  }
  bool mMessageScrollToLineCalled = false;
  void handleMessage(MessageScrollToLine* /*message*/) override {
    mMessageScrollToLineCalled = true;
  }
};

TEST_F(ActivationControllerFix, MessageChangeFileView) {
  const ActivationController controller(&storageAccess);
  const SpyMessageChangeFileView spyMessageChangeFileView;
  EXPECT_CALL(storageAccess, getNodeIdForFileNode(_)).WillOnce(Return(0));
  MessageActivateFile {FilePath {}, 10}.dispatchImmediately();
  std::this_thread::sleep_for(std::chrono::milliseconds {100});
  EXPECT_TRUE(spyMessageChangeFileView.mMessageChangeFileViewCalled);
  EXPECT_TRUE(spyMessageChangeFileView.mMessageScrollToLineCalled);
}

TEST_F(ActivationControllerFix, EmptyMessageActivateNodes) {
  const ActivationController controller(&storageAccess);
  EXPECT_CALL(storageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch> {}));
  const SpyMessageActivateTokens spyMessageActivateTokens;
  MessageActivateNodes {}.dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

TEST_F(ActivationControllerFix, MessageActivateNodes) {
  const ActivationController controller(&storageAccess);
  EXPECT_CALL(storageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch> {}));
  EXPECT_CALL(storageAccess, getNodeIdForNameHierarchy(_)).WillOnce(Return(1));
  const SpyMessageActivateTokens spyMessageActivateTokens;
  MessageActivateNodes messageActivateNodes;
  messageActivateNodes.nodes.resize(2);
  messageActivateNodes.nodes.back().nodeId = 1;
  messageActivateNodes.dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

TEST_F(ActivationControllerFix, MessageActivateTokenIds) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateTokens spyMessageActivateTokens;
  EXPECT_CALL(storageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch> {}));
  MessageActivateTokenIds {{}}.dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

struct SpyMessageActivateNodes : MessageListener<MessageActivateNodes> {
  bool mCalled = false;
  void handleMessage(MessageActivateNodes* /*message*/) override {
    mCalled = true;
  }
};

TEST_F(ActivationControllerFix, DISABLED_ActivateSourceLocations) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateNodes spyMessageActivateNodes;
  EXPECT_CALL(storageAccess, getNodeIdsForLocationIds(_)).WillOnce(Return(std::vector<Id> {}));
  EXPECT_CALL(storageAccess, getSearchMatchesForTokenIds(_)).WillOnce(Return(std::vector<SearchMatch> {}));

  MessageActivateSourceLocations {{}, false}.dispatchImmediately();

  std::this_thread::sleep_for(1s);

  EXPECT_TRUE(spyMessageActivateNodes.mCalled);
}

struct SpyMessageRefreshUI
    : MessageListener<MessageRefreshUI>
    , MessageListener<MessageStatus> {
  bool mMessageRefreshUICalled = false;
  void handleMessage(MessageRefreshUI* /*message*/) override {
    mMessageRefreshUICalled = true;
  }
  bool mMessageStatusCalled = false;
  void handleMessage(MessageStatus* /*message*/) override {
    mMessageStatusCalled = true;
  }
};

TEST_F(ActivationControllerFix, MessageResetZoom) {
  const ActivationController controller(&storageAccess);
  const SpyMessageRefreshUI spy;

  auto* settings = IApplicationSettings::getInstanceRaw();
  settings->setFontSizeStd(settings->getFontSizeStd() + 1);

  MessageResetZoom messageResetZoom;
  messageResetZoom.dispatchImmediately();

  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(spy.mMessageRefreshUICalled);
  EXPECT_TRUE(spy.mMessageStatusCalled);
}

TEST_F(ActivationControllerFix, MessageSearch) {
  const ActivationController controller(&storageAccess);
  const SpyMessageActivateTokens spyMessageActivateTokens;

  MessageSearch {{}, NodeTypeSet {}}.dispatchImmediately();
  EXPECT_TRUE(spyMessageActivateTokens.mCalled);
}

TEST_F(ActivationControllerFix, MessageZoom) {
  const ActivationController controller(&storageAccess);
  const SpyMessageRefreshUI spyMessageRefreshUI;

  MessageZoom {true}.dispatchImmediately();

  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(spyMessageRefreshUI.mMessageRefreshUICalled);
  EXPECT_TRUE(spyMessageRefreshUI.mMessageStatusCalled);
}