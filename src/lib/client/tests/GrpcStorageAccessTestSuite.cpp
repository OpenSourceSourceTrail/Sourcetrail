#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

#include "Capabilities.h"
#include "EngineChannel.h"
#include "EngineServiceImpl.h"
#include "GrpcStorageAccess.h"
#include "MockedStorageAccess.hpp"

#include "Graph.h"
#include "NameHierarchy.h"
#include "NodeType.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"
#include "TextAccess.h"

using testing::_;
using testing::Return;

namespace {

/**
 * A real engine server over a loopback socket, so these tests exercise the actual protos, the actual
 * EngineServiceImpl and the actual client -- the three pieces whose disagreement would be invisible
 * to a test that mocked the stub.
 */
class GrpcStorageAccessFix : public testing::Test {
protected:
  void SetUp() override {
    mService = std::make_unique<EngineServiceImpl>(&mStorage);

    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(mService.get());
    mServer = builder.BuildAndStart();
    ASSERT_NE(mServer, nullptr);

    mChannel = std::make_unique<EngineChannel>("127.0.0.1:" + std::to_string(port));
    // Keep the "engine is gone" tests quick; the production default is measured in seconds.
    mChannel->setCallTimeout(std::chrono::milliseconds(500));
    ASSERT_TRUE(mChannel->waitUntilReady(std::chrono::seconds(5)));

    mAccess = std::make_unique<GrpcStorageAccess>(mChannel.get());
  }

  void TearDown() override {
    stopServer();
    mAccess.reset();
    mChannel.reset();
    mService.reset();
  }

  void stopServer() {
    if(mServer) {
      mServer->Shutdown();
      mServer->Wait();
      mServer.reset();
    }
  }

  testing::NiceMock<MockedStorageAccess> mStorage;
  std::unique_ptr<EngineServiceImpl> mService;
  std::unique_ptr<grpc::Server> mServer;
  std::unique_ptr<EngineChannel> mChannel;
  std::unique_ptr<GrpcStorageAccess> mAccess;
};

}    // namespace

TEST_F(GrpcStorageAccessFix, scalarQueryRoundTrips) {
  EXPECT_CALL(mStorage, getNodeIdForFileNode(_)).WillOnce(Return(42));

  EXPECT_EQ(mAccess->getNodeIdForFileNode(FilePath(L"/src/main.cpp")), 42U);
}

TEST_F(GrpcStorageAccessFix, nameHierarchyRoundTrips) {
  NameHierarchy hierarchy(NAME_DELIMITER_CXX);
  hierarchy.push(L"ns");
  hierarchy.push(L"Klass");
  EXPECT_CALL(mStorage, getNameHierarchyForNodeId(7)).WillOnce(Return(hierarchy));

  EXPECT_EQ(mAccess->getNameHierarchyForNodeId(7).getQualifiedName(), hierarchy.getQualifiedName());
}

TEST_F(GrpcStorageAccessFix, graphRoundTrips) {
  auto graph = std::make_shared<Graph>();
  Node* from = graph->createNode(
      1, NodeType(NODE_CLASS), NameHierarchy(L"Base", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node* to = graph->createNode(
      2, NodeType(NODE_CLASS), NameHierarchy(L"Derived", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  ASSERT_NE(from, nullptr);
  ASSERT_NE(to, nullptr);
  graph->createEdge(3, Edge::EDGE_INHERITANCE, to, from);

  EXPECT_CALL(mStorage, getGraphForAll()).WillOnce(Return(graph));

  const auto restored = mAccess->getGraphForAll();

  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->getNodeCount(), 2U);
  EXPECT_EQ(restored->getEdgeCount(), 1U);
  ASSERT_NE(restored->getNodeById(1), nullptr);
  EXPECT_EQ(restored->getNodeById(1)->getName(), L"Base");
  ASSERT_NE(restored->getEdgeById(3), nullptr);
  EXPECT_EQ(restored->getEdgeById(3)->getType(), Edge::EDGE_INHERITANCE);
}

TEST_F(GrpcStorageAccessFix, sourceLocationsRoundTrip) {
  auto collection = std::make_shared<SourceLocationCollection>();
  collection->addSourceLocation(LOCATION_TOKEN, 11, {5}, FilePath(L"/src/main.cpp"), 2, 3, 2, 9);

  EXPECT_CALL(mStorage, getSourceLocationsForTokenIds(_)).WillOnce(Return(collection));

  const auto restored = mAccess->getSourceLocationsForTokenIds({5});

  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->getSourceLocationCount(), 1U);
  const SourceLocation* location = restored->getSourceLocationById(11);
  ASSERT_NE(location, nullptr);
  EXPECT_EQ(location->getType(), LOCATION_TOKEN);
  EXPECT_EQ(location->getTokenIds(), (std::vector<Id>{5}));
}

TEST_F(GrpcStorageAccessFix, searchMatchesRoundTrip) {
  SearchMatch match;
  match.name = L"Klass::method";
  match.text = L"method";
  match.nodeType = NodeType(NODE_METHOD);
  match.searchType = SearchMatch::SEARCH_TOKEN;
  match.tokenIds = {4};
  match.score = -3;

  EXPECT_CALL(mStorage, getAutocompletionMatches(_, _, _)).WillOnce(Return(std::vector<SearchMatch>{match}));

  const auto matches = mAccess->getAutocompletionMatches(L"met", NodeTypeSet::all(), true);

  ASSERT_EQ(matches.size(), 1U);
  EXPECT_EQ(matches[0].name, L"Klass::method");
  EXPECT_EQ(matches[0].nodeType.getKind(), NODE_METHOD);
  EXPECT_EQ(matches[0].searchType, SearchMatch::SEARCH_TOKEN);
  EXPECT_EQ(matches[0].tokenIds, (std::vector<Id>{4}));
  EXPECT_EQ(matches[0].score, -3);
}

TEST_F(GrpcStorageAccessFix, errorsRoundTrip) {
  const ErrorInfo error(1, L"boom", L"/src/a.cpp", 4, 5, L"/src/a.cpp", true, false);
  EXPECT_CALL(mStorage, getErrorsLimited(_)).WillOnce(Return(std::vector<ErrorInfo>{error}));

  const auto errors = mAccess->getErrorsLimited(ErrorFilter());

  ASSERT_EQ(errors.size(), 1U);
  EXPECT_EQ(errors[0].message, L"boom");
  EXPECT_TRUE(errors[0].fatal);
  EXPECT_FALSE(errors[0].indexed);
}

TEST_F(GrpcStorageAccessFix, fileContentRoundTrips) {
  EXPECT_CALL(mStorage, getFileContent(_, _)).WillOnce(Return(TextAccess::createFromString("int main() {}\n")));

  const auto content = mAccess->getFileContent(FilePath(L"/src/main.cpp"), false);

  ASSERT_NE(content, nullptr);
  EXPECT_EQ(content->getText(), "int main() {}\n");
}

// ---- The point of the whole class: a dead engine must not take the client with it ----------------

TEST_F(GrpcStorageAccessFix, queriesReturnEmptyResultsAfterEngineDies) {
  // Without these the test would pass vacuously: a NiceMock also answers with empty values, so
  // "empty result" alone cannot distinguish a failed call from a successful empty one. Requiring
  // that storage is never reached proves the calls really died in transport.
  EXPECT_CALL(mStorage, getNodeIdForFileNode(_)).Times(0);
  EXPECT_CALL(mStorage, getAutocompletionMatches(_, _, _)).Times(0);
  EXPECT_CALL(mStorage, getGraphForAll()).Times(0);
  EXPECT_CALL(mStorage, getFileContent(_, _)).Times(0);

  stopServer();

  // Not one of these may throw, abort, or return null.
  EXPECT_EQ(mAccess->getNodeIdForFileNode(FilePath(L"/src/main.cpp")), 0U);
  EXPECT_TRUE(mAccess->getNodeIdsForNameHierarchies({}).empty());
  EXPECT_TRUE(mAccess->getAutocompletionMatches(L"x", NodeTypeSet::all(), false).empty());
  EXPECT_TRUE(mAccess->getErrorsLimited(ErrorFilter()).empty());
  EXPECT_TRUE(mAccess->getAllNodeBookmarks().empty());
  EXPECT_EQ(mAccess->getErrorCount().total, 0U);

  const auto graph = mAccess->getGraphForAll();
  ASSERT_NE(graph, nullptr);
  EXPECT_EQ(graph->getNodeCount(), 0U);

  const auto collection = mAccess->getSourceLocationsForTokenIds({1});
  ASSERT_NE(collection, nullptr);
  EXPECT_EQ(collection->getSourceLocationCount(), 0U);

  const auto content = mAccess->getFileContent(FilePath(L"/src/main.cpp"), false);
  ASSERT_NE(content, nullptr);
  EXPECT_EQ(content->getLineCount(), 0U);

  EXPECT_FALSE(mChannel->isConnected());
}

TEST_F(GrpcStorageAccessFix, sourceLocationFileKeepsRequestedPathAfterEngineDies) {
  stopServer();

  // The code view titles the file from this path, so an empty result still has to carry it.
  const auto file = mAccess->getSourceLocationsForFile(FilePath(L"/src/main.cpp"));

  ASSERT_NE(file, nullptr);
  EXPECT_EQ(file->getFilePath().wstr(), L"/src/main.cpp");
  EXPECT_EQ(file->getSourceLocationCount(), 0U);
}

TEST_F(GrpcStorageAccessFix, mutationsAreSilentlyDroppedAfterEngineDies) {
  stopServer();

  EXPECT_EQ(mAccess->addBookmarkCategory(L"favourites"), 0U);
  EXPECT_NO_FATAL_FAILURE(mAccess->removeBookmark(1));
  EXPECT_NO_FATAL_FAILURE(mAccess->updateBookmark(1, L"n", L"c", L"cat"));
}

TEST_F(GrpcStorageAccessFix, channelIsMarkedDegradedOnFailureAndReportsIt) {
  bool connected = true;
  int transitions = 0;
  mChannel->setConnectionStateHandler([&](bool state) {
    connected = state;
    ++transitions;
  });

  EXPECT_CALL(mStorage, getNodeIdForFileNode(_)).WillOnce(Return(1));
  EXPECT_EQ(mAccess->getNodeIdForFileNode(FilePath(L"/a")), 1U);
  EXPECT_TRUE(mChannel->isConnected());

  stopServer();
  mAccess->getNodeIdForFileNode(FilePath(L"/a"));

  EXPECT_FALSE(mChannel->isConnected());
  EXPECT_FALSE(connected);
  EXPECT_EQ(transitions, 1);
}

TEST_F(GrpcStorageAccessFix, capabilitiesAreServedAndGoEmptyWithoutAnEngine) {
  client::Capabilities& capabilities = client::Capabilities::instance();
  capabilities.setChannel(mChannel.get());

  // No plugin is discovered in this fixture, which is precisely the bare-install case: Custom Command
  // still works because the user supplies the command line, so a project can still be created.
  EXPECT_TRUE(capabilities.supportsSourceGroupType(SOURCE_GROUP_CUSTOM_COMMAND));
  EXPECT_TRUE(capabilities.canCreateProject());

  stopServer();
  capabilities.invalidate();

  // Read-only mode: no engine, no capabilities, and no crash.
  EXPECT_FALSE(capabilities.canCreateProject());
  EXPECT_FALSE(capabilities.supportsSourceGroupType(SOURCE_GROUP_CUSTOM_COMMAND));

  capabilities.setChannel(nullptr);
}
