#include <gtest/gtest.h>

#include "ConvertGraph.h"
#include "Edge.h"
#include "Graph.h"
#include "NameHierarchy.h"
#include "Node.h"
#include "NodeType.h"
#include "TokenComponentAccess.h"
#include "TokenComponentBundledEdges.h"
#include "TokenComponentFilePath.h"
#include "TokenComponentInheritanceChain.h"
#include "TokenComponentIsAmbiguous.h"

using namespace proto::convert;

namespace {

NameHierarchy makeName(const std::wstring& name) {
  NameHierarchy hierarchy(NAME_DELIMITER_CXX);
  hierarchy.push(name);
  return hierarchy;
}

Node* addNode(Graph& graph, Id id, const std::wstring& name, NodeKind kind = NODE_CLASS) {
  return graph.createNode(id, NodeType(kind), makeName(name), DEFINITION_EXPLICIT);
}

}    // namespace

TEST(ConvertGraph, emptyGraphRoundTrips) {
  const Graph graph;

  const auto restored = fromProto(toProto(graph));

  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->getNodeCount(), 0U);
  EXPECT_EQ(restored->getEdgeCount(), 0U);
}

TEST(ConvertGraph, nodesAndEdgesRoundTrip) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");
  nodeA->setChildCount(7);
  graph.createEdge(10, Edge::EDGE_CALL, nodeA, nodeB);

  const auto restored = fromProto(toProto(graph));

  ASSERT_EQ(restored->getNodeCount(), 2U);
  ASSERT_EQ(restored->getEdgeCount(), 1U);

  const Node* restoredA = restored->getNodeById(1);
  ASSERT_NE(restoredA, nullptr);
  EXPECT_EQ(restoredA->getName(), L"A");
  EXPECT_EQ(restoredA->getType().getKind(), NODE_CLASS);
  EXPECT_TRUE(restoredA->isExplicit());
  EXPECT_EQ(restoredA->getChildCount(), 7U);

  const Edge* restoredEdge = restored->getEdgeById(10);
  ASSERT_NE(restoredEdge, nullptr);
  EXPECT_EQ(restoredEdge->getType(), Edge::EDGE_CALL);
  EXPECT_EQ(restoredEdge->getFrom()->getId(), 1U);
  EXPECT_EQ(restoredEdge->getTo()->getId(), 2U);
}

TEST(ConvertGraph, definitionKindRoundTrips) {
  Graph graph;
  graph.createNode(1, NodeType(NODE_CLASS), makeName(L"none"), DEFINITION_NONE);
  graph.createNode(2, NodeType(NODE_CLASS), makeName(L"implicit"), DEFINITION_IMPLICIT);
  graph.createNode(3, NodeType(NODE_CLASS), makeName(L"explicit"), DEFINITION_EXPLICIT);

  const auto restored = fromProto(toProto(graph));

  EXPECT_FALSE(restored->getNodeById(1)->isDefined());
  EXPECT_TRUE(restored->getNodeById(2)->isImplicit());
  EXPECT_TRUE(restored->getNodeById(3)->isExplicit());
}

TEST(ConvertGraph, accessComponentRoundTrips) {
  Graph graph;
  addNode(graph, 1, L"withAccess")->addComponent(std::make_shared<TokenComponentAccess>(ACCESS_PROTECTED));
  addNode(graph, 2, L"withoutAccess");

  const auto restored = fromProto(toProto(graph));

  const auto* access = restored->getNodeById(1)->getComponent<TokenComponentAccess>();
  ASSERT_NE(access, nullptr);
  EXPECT_EQ(access->getAccess(), ACCESS_PROTECTED);
  // Absent components must stay absent rather than materialising as a default.
  EXPECT_EQ(restored->getNodeById(2)->getComponent<TokenComponentAccess>(), nullptr);
}

TEST(ConvertGraph, filePathComponentRoundTrips) {
  Graph graph;
  addNode(graph, 1, L"complete")->addComponent(std::make_shared<TokenComponentFilePath>(FilePath(L"/src/main.cpp"), true));
  addNode(graph, 2, L"incomplete")->addComponent(std::make_shared<TokenComponentFilePath>(FilePath(L"/src/other.cpp"), false));

  const auto restored = fromProto(toProto(graph));

  const auto* complete = restored->getNodeById(1)->getComponent<TokenComponentFilePath>();
  ASSERT_NE(complete, nullptr);
  EXPECT_EQ(complete->getFilePath().wstr(), L"/src/main.cpp");
  EXPECT_TRUE(complete->isComplete());

  const auto* incomplete = restored->getNodeById(2)->getComponent<TokenComponentFilePath>();
  ASSERT_NE(incomplete, nullptr);
  EXPECT_FALSE(incomplete->isComplete());
}

TEST(ConvertGraph, isAmbiguousComponentRoundTrips) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");
  graph.createEdge(10, Edge::EDGE_CALL, nodeA, nodeB)->addComponent(std::make_shared<TokenComponentIsAmbiguous>());
  graph.createEdge(11, Edge::EDGE_CALL, nodeB, nodeA);

  const auto restored = fromProto(toProto(graph));

  EXPECT_NE(restored->getEdgeById(10)->getComponent<TokenComponentIsAmbiguous>(), nullptr);
  EXPECT_EQ(restored->getEdgeById(11)->getComponent<TokenComponentIsAmbiguous>(), nullptr);
}

TEST(ConvertGraph, inheritanceChainComponentRoundTrips) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");
  graph.createEdge(10, Edge::EDGE_INHERITANCE, nodeA, nodeB)
      ->addComponent(std::make_shared<TokenComponentInheritanceChain>(std::vector<Id>{41, 42, 43}));

  const auto restored = fromProto(toProto(graph));

  const auto* chain = restored->getEdgeById(10)->getComponent<TokenComponentInheritanceChain>();
  ASSERT_NE(chain, nullptr);
  EXPECT_EQ(chain->inheritanceEdgeIds, (std::vector<Id>{41, 42, 43}));
}

// The GUI dereferences this component without a null check (DummyEdge, GraphController), so an
// empty-but-present bundle must survive as present.
TEST(ConvertGraph, emptyBundledEdgesComponentStaysPresent) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");
  graph.createEdge(10, Edge::EDGE_BUNDLED_EDGES, nodeA, nodeB)->addComponent(std::make_shared<TokenComponentBundledEdges>());

  const auto restored = fromProto(toProto(graph));

  auto* bundled = restored->getEdgeById(10)->getComponent<TokenComponentBundledEdges>();
  ASSERT_NE(bundled, nullptr);
  EXPECT_EQ(bundled->getBundledEdgesCount(), 0);
}

TEST(ConvertGraph, bundledEdgesDirectionRoundTrips) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");

  auto allForward = std::make_shared<TokenComponentBundledEdges>();
  allForward->addBundledEdgesId(100, true);
  allForward->addBundledEdgesId(101, true);
  graph.createEdge(10, Edge::EDGE_BUNDLED_EDGES, nodeA, nodeB)->addComponent(allForward);

  auto allBackward = std::make_shared<TokenComponentBundledEdges>();
  allBackward->addBundledEdgesId(200, false);
  graph.createEdge(11, Edge::EDGE_BUNDLED_EDGES, nodeB, nodeA)->addComponent(allBackward);

  // Mixed directions collapse to DIRECTION_NONE, which is only reproducible if each id's direction
  // survives the wire individually.
  auto mixed = std::make_shared<TokenComponentBundledEdges>();
  mixed->addBundledEdgesId(300, true);
  mixed->addBundledEdgesId(301, false);
  graph.createEdge(12, Edge::EDGE_BUNDLED_EDGES, nodeA, nodeB)->addComponent(mixed);

  const auto restored = fromProto(toProto(graph));

  auto* restoredForward = restored->getEdgeById(10)->getComponent<TokenComponentBundledEdges>();
  ASSERT_NE(restoredForward, nullptr);
  EXPECT_EQ(restoredForward->getBundledEdgesCount(), 2);
  EXPECT_EQ(restoredForward->getBundledEdgesIds(), (std::set<Id>{100, 101}));
  EXPECT_EQ(restoredForward->getDirection(), TokenComponentBundledEdges::DIRECTION_FORWARD);

  auto* restoredBackward = restored->getEdgeById(11)->getComponent<TokenComponentBundledEdges>();
  ASSERT_NE(restoredBackward, nullptr);
  EXPECT_EQ(restoredBackward->getDirection(), TokenComponentBundledEdges::DIRECTION_BACKWARD);

  auto* restoredMixed = restored->getEdgeById(12)->getComponent<TokenComponentBundledEdges>();
  ASSERT_NE(restoredMixed, nullptr);
  EXPECT_EQ(restoredMixed->getDirection(), TokenComponentBundledEdges::DIRECTION_NONE);
}

TEST(ConvertGraph, trailModeAndOriginRoundTrip) {
  Graph graph;
  graph.setTrailMode(Graph::TRAIL_VERTICAL);
  graph.setHasTrailOrigin(true);

  const auto restored = fromProto(toProto(graph));

  EXPECT_EQ(restored->getTrailMode(), Graph::TRAIL_VERTICAL);
  EXPECT_TRUE(restored->hasTrailOrigin());
}

// A message whose edge references a node that did not survive must be skipped, not turned into an
// edge with null endpoints.
TEST(ConvertGraph, edgeWithUnknownEndpointIsSkipped) {
  Graph graph;
  Node* nodeA = addNode(graph, 1, L"A");
  Node* nodeB = addNode(graph, 2, L"B");
  graph.createEdge(10, Edge::EDGE_CALL, nodeA, nodeB);

  auto msg = toProto(graph);
  msg.mutable_edges(0)->set_to_node_id(9999);

  const auto restored = fromProto(msg);

  EXPECT_EQ(restored->getEdgeCount(), 0U);
  EXPECT_EQ(restored->getNodeCount(), 2U);
}
