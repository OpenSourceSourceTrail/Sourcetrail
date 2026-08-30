#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component/controller/helper/DummyEdge.h"
#include "component/controller/helper/DummyNode.h"
#include "component/view/GraphViewStyle.h"
#include "component/view/MetricGraphViewStyleImpl.h"
#include "graph/QmlGraphView.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"

using namespace testing;
using graph::NodeItem;
using graph::Snapshot;

namespace {

constexpr float CharWidth = 7.0F;
constexpr float CharHeight = 14.0F;
constexpr size_t ReferenceFontSize = 14;

std::shared_ptr<DummyNode> makeNode(DummyNode::Type type, Id tokenId, Vec2f position, Vec2f size) {
  auto node = std::make_shared<DummyNode>(type);
  node->visible = true;
  node->tokenId = tokenId;
  node->position = position;
  node->size = size;
  return node;
}

/** Runs one tree through the view and returns what it handed to the GUI thread. */
Snapshot flattenTree(const std::vector<std::shared_ptr<DummyNode>>& nodes,
                     const std::vector<std::shared_ptr<DummyEdge>>& edges = {}) {
  Snapshot captured;
  graph::QmlGraphView view{[&captured](Snapshot snapshot) { captured = std::move(snapshot); }};
  view.rebuildGraph(nullptr, nodes, edges, GraphView::GraphParams{});
  return captured;
}

struct GraphSnapshotFix : Test {
  void SetUp() override {
    // loadStyleSettings reads the font name and size straight off the settings singleton, so the
    // styles cannot be resolved without one. ColorScheme creates itself on first use.
    IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());

    // Node boxes are sized from font metrics; without an impl there is nothing to measure with.
    GraphViewStyle::setImpl(std::make_shared<MetricGraphViewStyleImpl>(CharWidth, CharHeight, ReferenceFontSize));
    GraphViewStyle::loadStyleSettings();
  }

  void TearDown() override {
    GraphViewStyle::setImpl(nullptr);
    IApplicationSettings::setInstance(nullptr);
  }
};

}    // namespace

// The one thing a flat model must get right: DummyNode positions are relative to the parent
// (DummyNode::getActiveSubNodeRect accumulates them), and the scene needs absolute ones.
TEST_F(GraphSnapshotFix, subNodePositionsBecomeAbsolute) {
  auto parent = makeNode(DummyNode::DUMMY_GROUP, 1, {100, 200}, {300, 400});
  auto child = makeNode(DummyNode::DUMMY_ACCESS, 0, {10, 20}, {80, 30});
  auto grandchild = makeNode(DummyNode::DUMMY_BUNDLE, 2, {5, 5}, {40, 20});
  child->subNodes.push_back(grandchild);
  parent->subNodes.push_back(child);

  const Snapshot snapshot = flattenTree({parent});

  ASSERT_THAT(snapshot.nodes, SizeIs(3));
  EXPECT_THAT(snapshot.nodes[0].x, DoubleEq(100));
  EXPECT_THAT(snapshot.nodes[0].y, DoubleEq(200));
  EXPECT_THAT(snapshot.nodes[1].x, DoubleEq(110));
  EXPECT_THAT(snapshot.nodes[1].y, DoubleEq(220));
  EXPECT_THAT(snapshot.nodes[2].x, DoubleEq(115));
  EXPECT_THAT(snapshot.nodes[2].y, DoubleEq(225));
}

// Pre-order is also back-to-front paint order: an access section draws over the box holding it.
TEST_F(GraphSnapshotFix, nodesAreFlattenedInPreOrder) {
  auto parent = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 100});
  parent->subNodes.push_back(makeNode(DummyNode::DUMMY_BUNDLE, 2, {0, 0}, {10, 10}));
  parent->subNodes.push_back(makeNode(DummyNode::DUMMY_BUNDLE, 3, {0, 0}, {10, 10}));

  const Snapshot snapshot = flattenTree({parent});

  ASSERT_THAT(snapshot.nodes, SizeIs(3));
  EXPECT_THAT(snapshot.nodes[0].tokenId, Eq(1U));
  EXPECT_THAT(snapshot.nodes[1].tokenId, Eq(2U));
  EXPECT_THAT(snapshot.nodes[2].tokenId, Eq(3U));
}

TEST_F(GraphSnapshotFix, invisibleNodesAreDropped) {
  auto parent = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 100});
  auto hidden = makeNode(DummyNode::DUMMY_BUNDLE, 2, {0, 0}, {10, 10});
  hidden->visible = false;
  parent->subNodes.push_back(hidden);

  EXPECT_THAT(flattenTree({parent}).nodes, SizeIs(1));
}

// An expand toggle carries no token of its own, so a click on it has to act on the enclosing box.
TEST_F(GraphSnapshotFix, expandToggleCarriesTheEnclosingNodesToken) {
  auto parent = makeNode(DummyNode::DUMMY_GROUP, 42, {0, 0}, {100, 100});
  parent->subNodes.push_back(makeNode(DummyNode::DUMMY_EXPAND_TOGGLE, 0, {0, 90}, {100, 10}));

  const Snapshot snapshot = flattenTree({parent});

  ASSERT_THAT(snapshot.nodes, SizeIs(2));
  EXPECT_THAT(snapshot.nodes[1].type, Eq(static_cast<int>(DummyNode::DUMMY_EXPAND_TOGGLE)));
  EXPECT_THAT(snapshot.nodes[1].ownerTokenId, Eq(42U));
}

TEST_F(GraphSnapshotFix, boundsCoverEveryNode) {
  const Snapshot snapshot = flattenTree(
      {makeNode(DummyNode::DUMMY_GROUP, 1, {-50, -20}, {100, 40}), makeNode(DummyNode::DUMMY_GROUP, 2, {200, 300}, {100, 40})});

  EXPECT_THAT(snapshot.boundsX, DoubleEq(-50));
  EXPECT_THAT(snapshot.boundsY, DoubleEq(-20));
  EXPECT_THAT(snapshot.boundsWidth, DoubleEq(350));
  EXPECT_THAT(snapshot.boundsHeight, DoubleEq(360));
}

// A bundled edge has no Edge behind it, so every one of them would report token id 0; the row id is
// what QML hands back, and it has to stay unique or clicks land on the wrong edge.
TEST_F(GraphSnapshotFix, edgesGetDistinctRowIdsEvenWithoutTokens) {
  auto left = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 50});
  auto middle = makeNode(DummyNode::DUMMY_GROUP, 2, {300, 0}, {100, 50});
  auto right = makeNode(DummyNode::DUMMY_GROUP, 3, {600, 0}, {100, 50});

  auto first = std::make_shared<DummyEdge>(1, 2, nullptr);
  first->visible = true;
  auto second = std::make_shared<DummyEdge>(2, 3, nullptr);
  second->visible = true;

  const Snapshot snapshot = flattenTree({left, middle, right}, {first, second});

  ASSERT_THAT(snapshot.edges, SizeIs(2));
  EXPECT_THAT(snapshot.edges[0].id, Ne(snapshot.edges[1].id));
  EXPECT_THAT(snapshot.edgeActivations, SizeIs(2));
}

TEST_F(GraphSnapshotFix, edgesBetweenLaidOutNodesGetAPath) {
  auto owner = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 50});
  auto target = makeNode(DummyNode::DUMMY_GROUP, 2, {400, 0}, {100, 50});

  auto edge = std::make_shared<DummyEdge>(1, 2, nullptr);
  edge->visible = true;

  const Snapshot snapshot = flattenTree({owner, target}, {edge});

  ASSERT_THAT(snapshot.edges, SizeIs(1));
  // An SVG "d" string always opens with a moveto; anything else means the path never got built.
  EXPECT_THAT(snapshot.edges[0].path.toStdString(), StartsWith("M"));
  EXPECT_THAT(snapshot.edges[0].path.toStdString(), Not(IsEmpty()));
}

TEST_F(GraphSnapshotFix, edgesToNodesThatWereNotLaidOutAreSkipped) {
  auto owner = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 50});

  auto edge = std::make_shared<DummyEdge>(1, 99, nullptr);
  edge->visible = true;

  EXPECT_THAT(flattenTree({owner}, {edge}).edges, IsEmpty());
}

TEST_F(GraphSnapshotFix, invisibleEdgesAreDropped) {
  auto owner = makeNode(DummyNode::DUMMY_GROUP, 1, {0, 0}, {100, 50});
  auto target = makeNode(DummyNode::DUMMY_GROUP, 2, {400, 0}, {100, 50});

  auto edge = std::make_shared<DummyEdge>(1, 2, nullptr);
  edge->visible = false;

  EXPECT_THAT(flattenTree({owner, target}, {edge}).edges, IsEmpty());
}

TEST_F(GraphSnapshotFix, clearEmitsAnEmptySnapshot) {
  Snapshot captured;
  captured.nodes.emplace_back();

  graph::QmlGraphView view{[&captured](Snapshot snapshot) { captured = std::move(snapshot); }};
  view.clear();

  EXPECT_THAT(captured.nodes, IsEmpty());
  EXPECT_THAT(captured.edges, IsEmpty());
}
