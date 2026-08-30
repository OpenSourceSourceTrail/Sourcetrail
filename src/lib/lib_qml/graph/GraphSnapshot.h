#pragma once
#include <map>
#include <vector>

#include <QColor>
#include <QString>
#include <QUrl>

#include "data/graph/Edge.h"
#include "data/name/NameHierarchy.h"
#include "GlobalId.hpp"

namespace graph {

/**
 * One painted node, flattened out of the DummyNode tree.
 *
 * DummyNode positions are relative to the parent node (see DummyNode::getActiveSubNodeRect, which
 * accumulates them); these are absolute, so the scene is a flat list and viewport culling is a
 * rectangle test. Nodes appear in pre-order, which is also back-to-front paint order: an access
 * section is drawn over the class box that contains it.
 *
 * The style is resolved here rather than in QML. GraphViewStyle already knows what a public method
 * of an implicit template class looks like, and it is the same answer the widget GUI painted -- QML
 * asking those questions again in its own vocabulary is how two renderers drift apart.
 */
struct NodeItem final {
  int type = 0;    ///< DummyNode::Type
  Id tokenId = 0;
  /// The enclosing node's token. An expand toggle carries no token of its own but acts on its box.
  Id ownerTokenId = 0;

  QString name;
  qreal x = 0;
  qreal y = 0;
  qreal width = 0;
  qreal height = 0;

  QColor fillColor;
  QColor borderColor;
  QColor textColor;

  int cornerRadius = 0;
  int borderWidth = 0;
  bool borderDashed = false;

  QString fontFamily;
  int fontSize = 0;
  bool fontBold = false;
  qreal textOffsetX = 0;
  qreal textOffsetY = 0;

  QUrl iconSource;
  qreal iconOffsetX = 0;
  qreal iconOffsetY = 0;
  int iconSize = 0;

  bool active = false;
  bool connected = false;
  bool expanded = false;
  bool childVisible = false;
  bool interactive = true;
  bool hasHatching = false;

  /// Expand toggles show this count when collapsed, and an arrow when expanded.
  int invisibleSubNodeCount = 0;
  /// Bundle nodes show this in a count circle.
  int bundledNodeCount = 0;
  bool hasMissingChildNodes = false;
};

/** One painted edge. The path is an SVG "d" string in the same absolute coordinates as the nodes. */
struct EdgeItem final {
  /// Row identity. Not the token id: bundled edges carry no Edge and would all collide on zero.
  Id id = 0;
  Id tokenId = 0;
  Id ownerId = 0;
  Id targetId = 0;

  QString path;
  QString arrowPath;    ///< Empty unless the style draws a detached (dashed-line) arrow head.

  QColor color;
  qreal width = 1;
  bool dashed = false;
  bool active = false;
  int weight = 1;
  int zValue = 0;
};

/**
 * What clicking an edge should dispatch, worked out while the Edge* was still in reach.
 *
 * QML only ever hands back a token id, and by then the Graph the edge came from may be gone. So the
 * message is prepared here instead of looked up later -- the same fields QtGraphEdge::onClick read
 * straight off its Edge pointer.
 */
struct EdgeActivation final {
  Edge::EdgeType type = Edge::EDGE_BUNDLED_EDGES;
  NameHierarchy sourceName{NAME_DELIMITER_UNKNOWN};
  NameHierarchy targetName{NAME_DELIMITER_UNKNOWN};
  std::vector<Id> bundledEdgeIds;

  /// An edge into or out of a group splits that group instead of activating.
  bool expandable = false;
  Id expandTokenId = 0;
};

/** Everything one rebuildGraph() call produced, ready to cross to the GUI thread by value. */
struct Snapshot final {
  std::vector<NodeItem> nodes;
  std::vector<EdgeItem> edges;
  std::map<Id, EdgeActivation> edgeActivations;

  qreal boundsX = 0;
  qreal boundsY = 0;
  qreal boundsWidth = 0;
  qreal boundsHeight = 0;

  Id activeTokenId = 0;
  bool centerActiveNode = false;
};

}    // namespace graph
