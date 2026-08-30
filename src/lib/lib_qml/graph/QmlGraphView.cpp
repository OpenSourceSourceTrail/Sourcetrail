#include "graph/QmlGraphView.h"

#include <algorithm>
#include <limits>
#include <map>

#include <QUrl>

#include "app/paths/ResourcePaths.h"
#include "component/controller/helper/DummyEdge.h"
#include "component/controller/helper/DummyNode.h"
#include "component/view/GraphViewStyle.h"
#include "data/graph/Edge.h"
#include "data/graph/Graph.h"
#include "data/graph/Node.h"
#include "data/graph/token_component/TokenComponentAccess.h"
#include "data/graph/token_component/TokenComponentBundledEdges.h"
#include "data/graph/token_component/TokenComponentInheritanceChain.h"
#include "graph/EdgeRouter.h"
#include "utilityString.h"

namespace graph {

namespace {

constexpr float DefaultViewWidth = 1200.0F;
constexpr float DefaultViewHeight = 800.0F;
constexpr int AccessIconSize = 16;

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

QColor toColor(const std::string& value) {
  return value.empty() ? QColor{} : QColor{QString::fromStdString(value)};
}

QUrl guiImage(const std::wstring& relativePath) {
  return QUrl::fromLocalFile(QString::fromStdString(ResourcePaths::getGuiDirectoryPath().concatenate(relativePath).str()));
}

QUrl accessIcon(AccessKind access) {
  switch(access) {
  case ACCESS_PUBLIC:
    return guiImage(L"graph_view/images/public.png");
  case ACCESS_PROTECTED:
    return guiImage(L"graph_view/images/protected.png");
  case ACCESS_PRIVATE:
    return guiImage(L"graph_view/images/private.png");
  case ACCESS_DEFAULT:
    return guiImage(L"graph_view/images/default.png");
  case ACCESS_TEMPLATE_PARAMETER:
  case ACCESS_TYPE_PARAMETER:
    return guiImage(L"graph_view/images/template.png");
  default:
    return {};
  }
}

/** Copies one resolved NodeStyle onto the item QML paints from. */
void applyStyle(NodeItem& item, const GraphViewStyle::NodeStyle& style) {
  item.fillColor = toColor(style.color.fill);
  item.borderColor = toColor(style.color.border);
  item.textColor = toColor(style.color.text);
  item.cornerRadius = style.cornerRadius;
  item.borderWidth = style.borderWidth;
  item.borderDashed = style.borderDashed;
  item.fontFamily = QString::fromStdString(style.fontName);
  item.fontSize = static_cast<int>(style.fontSize);
  item.fontBold = style.fontBold;
  item.textOffsetX = style.textOffset.x;
  item.textOffsetY = style.textOffset.y;
  item.iconOffsetX = style.iconOffset.x;
  item.iconOffsetY = style.iconOffset.y;
  item.iconSize = static_cast<int>(style.iconSize);
  item.hasHatching = style.hasHatching;
  if(!style.iconPath.empty()) {
    item.iconSource = QUrl::fromLocalFile(QString::fromStdString(style.iconPath.str()));
  }
}

/** Where an edge end sits, and which enclosing boxes it has to clear. */
struct NodeGeometry final {
  Rect rect;
  Rect lastParent;
  Rect lastNonGroupParent;
  bool isGroup = false;
};

struct FlattenState final {
  std::vector<NodeItem> nodes;
  std::map<Id, NodeGeometry> geometry;
  bool multipleActive = false;
};

void flatten(FlattenState& state,
             const DummyNode& node,
             Vec2f parentOrigin,
             const Rect& lastParent,
             const Rect& lastNonGroupParent,
             bool rootLevel,
             Id ownerTokenId) {
  if(!node.visible) {
    return;
  }

  // DummyNode positions are relative to the parent; the scene wants absolute ones.
  const Vec2f origin = parentOrigin + node.position;
  const Rect rect{origin.x, origin.y, origin.x + node.size.x, origin.y + node.size.y};

  // A root node is its own outermost ancestor, which is what the edge router expects.
  const Rect effectiveParent = rootLevel ? rect : lastParent;
  const Rect effectiveNonGroupParent = (rootLevel || node.isGroupNode()) ? rect : lastNonGroupParent;

  NodeItem item;
  item.type = static_cast<int>(node.type);
  item.tokenId = node.tokenId;
  item.ownerTokenId = ownerTokenId;
  item.name = toQt(node.name);
  item.x = origin.x;
  item.y = origin.y;
  item.width = node.size.x;
  item.height = node.size.y;
  item.active = node.active;
  item.connected = node.connected;
  item.expanded = node.expanded;
  item.childVisible = node.childVisible;
  item.interactive = node.interactive;

  switch(node.type) {
  case DummyNode::DUMMY_DATA: {
    applyStyle(item,
               GraphViewStyle::getStyleForNodeType(node.data->getType(),
                                                   node.data->isExplicit(),
                                                   node.active,
                                                   false,
                                                   false,
                                                   node.childVisible,
                                                   node.getQualifierNode() != nullptr));
    item.hasMissingChildNodes = node.hasMissingChildNodes();
    break;
  }
  case DummyNode::DUMMY_ACCESS:
    applyStyle(item, GraphViewStyle::getStyleOfAccessNode());
    item.name = toQt(TokenComponentAccess::getAccessString(node.accessKind));
    item.iconSource = accessIcon(node.accessKind);
    item.iconSize = AccessIconSize;
    break;
  case DummyNode::DUMMY_EXPAND_TOGGLE:
    applyStyle(item, GraphViewStyle::getStyleOfExpandToggleNode());
    item.invisibleSubNodeCount = static_cast<int>(node.invisibleSubNodeCount);
    break;
  case DummyNode::DUMMY_BUNDLE:
    applyStyle(item,
               node.bundledNodeType.isUnknownSymbol() ?
                   GraphViewStyle::getStyleOfBundleNode(false) :
                   GraphViewStyle::getStyleForNodeType(node.bundledNodeType, true, false, false, false, false, false));
    item.bundledNodeCount = static_cast<int>(node.getBundledNodeCount());
    break;
  case DummyNode::DUMMY_QUALIFIER:
    applyStyle(item, GraphViewStyle::getStyleOfQualifier());
    item.name = toQt(node.qualifierName.getQualifiedName());
    break;
  case DummyNode::DUMMY_TEXT:
    applyStyle(item, GraphViewStyle::getStyleOfTextNode(node.fontSizeDiff));
    break;
  case DummyNode::DUMMY_GROUP:
    applyStyle(item, GraphViewStyle::getStyleOfGroupNode(node.groupType, false));
    break;
  default:
    break;
  }

  state.nodes.push_back(std::move(item));

  // Edges are addressed by token id; the first node carrying one wins, as findNodeRecursive did.
  if(node.tokenId != 0 && state.geometry.find(node.tokenId) == state.geometry.end()) {
    state.geometry.emplace(node.tokenId, NodeGeometry{rect, effectiveParent, effectiveNonGroupParent, node.isGroupNode()});
  }

  const Id childOwner = node.tokenId != 0 ? node.tokenId : ownerTokenId;
  for(const std::shared_ptr<DummyNode>& subNode : node.subNodes) {
    flatten(state, *subNode, origin, effectiveParent, effectiveNonGroupParent, false, childOwner);
  }
}

}    // namespace

QmlGraphView::QmlGraphView(SnapshotHandler onSnapshot)
    : GraphView(nullptr), mOnSnapshot(std::move(onSnapshot)), mViewSize(DefaultViewWidth, DefaultViewHeight) {}

QmlGraphView::~QmlGraphView() = default;

void QmlGraphView::setLayoutInputs(Vec2f viewSize, GroupType grouping) {
  mViewSize = viewSize;
  mGrouping = grouping;
}

void QmlGraphView::refreshView() {}

void QmlGraphView::rebuildGraph(std::shared_ptr<Graph> graph,
                                const std::vector<std::shared_ptr<DummyNode>>& nodes,
                                const std::vector<std::shared_ptr<DummyEdge>>& edges,
                                const GraphParams params) {
  FlattenState state;
  size_t activeCount = 0;
  for(const std::shared_ptr<DummyNode>& node : nodes) {
    activeCount += node->getActiveSubNodeCount();
  }
  state.multipleActive = activeCount > 1;

  for(const std::shared_ptr<DummyNode>& node : nodes) {
    flatten(state, *node, {}, {}, {}, true, 0);
  }

  Snapshot snapshot;
  snapshot.nodes = std::move(state.nodes);
  snapshot.centerActiveNode = params.centerActiveNode;

  const Route route = params.bezierEdges ? Route::Horizontal : Route::Any;
  const bool trailMode = graph && graph->getTrailMode() != Graph::TRAIL_NONE;

  for(const std::shared_ptr<DummyEdge>& edge : edges) {
    if(!edge->visible) {
      continue;
    }

    const auto owner = state.geometry.find(edge->ownerId);
    const auto target = state.geometry.find(edge->targetId);
    if(owner == state.geometry.end() || target == state.geometry.end()) {
      continue;
    }

    const Edge::EdgeType edgeType = edge->data != nullptr ? edge->data->getType() : Edge::EDGE_BUNDLED_EDGES;
    const GraphViewStyle::EdgeStyle style = GraphViewStyle::getStyleForEdgeType(edgeType, edge->active, false, trailMode, false);

    // Two ends inside the same group are routed against their own boxes, not the group's.
    const bool sameGroup = owner->second.lastParent.left == target->second.lastParent.left &&
        owner->second.lastParent.top == target->second.lastParent.top;
    const Rect ownerParent = sameGroup ? owner->second.lastNonGroupParent : owner->second.lastParent;
    const Rect targetParent = sameGroup ? target->second.lastNonGroupParent : target->second.lastParent;

    const auto poly = routeEdge(
        owner->second.rect, target->second.rect, ownerParent, targetParent, style, route, false, false, false);
    const EdgePath path = buildEdgePath(poly, style, edge->getDirection() != TokenComponentBundledEdges::DIRECTION_NONE);

    EdgeItem item;
    item.id = static_cast<Id>(snapshot.edges.size()) + 1;
    item.tokenId = edge->data != nullptr ? edge->data->getId() : 0;

    // Worked out now, while the Edge is still reachable; QML only hands a token id back.
    EdgeActivation activation;
    activation.expandable = edge->data == nullptr || owner->second.isGroup || target->second.isGroup;
    if(activation.expandable) {
      Id expandId = edge->getDirection() == TokenComponentBundledEdges::DIRECTION_BACKWARD ? edge->ownerId : edge->targetId;
      if(owner->second.isGroup) {
        expandId = edge->ownerId;
      } else if(target->second.isGroup) {
        expandId = edge->targetId;
      }
      activation.expandTokenId = expandId;
    } else {
      auto* inheritance = edge->data->getComponent<TokenComponentInheritanceChain>();
      activation.type = inheritance != nullptr ? Edge::EDGE_BUNDLED_EDGES : edge->data->getType();
      activation.sourceName = edge->data->getFrom()->getNameHierarchy();
      activation.targetName = edge->data->getTo()->getNameHierarchy();
      if(edge->data->getType() == Edge::EDGE_BUNDLED_EDGES) {
        const auto ids = edge->data->getComponent<TokenComponentBundledEdges>()->getBundledEdgesIds();
        activation.bundledEdgeIds.assign(ids.begin(), ids.end());
      } else if(inheritance != nullptr) {
        activation.bundledEdgeIds = inheritance->inheritanceEdgeIds;
      }
    }
    snapshot.edgeActivations.emplace(item.id, std::move(activation));

    item.ownerId = edge->ownerId;
    item.targetId = edge->targetId;
    item.path = path.path;
    item.arrowPath = path.arrowPath;
    item.color = toColor(style.color);
    // The weight widens the line the same way the widget pen did.
    item.width = static_cast<qreal>(style.width) + std::log10(static_cast<double>(std::max(edge->getWeight(), 1)));
    item.dashed = style.dashed;
    item.active = edge->active;
    item.weight = edge->getWeight();
    item.zValue = style.zValue;
    snapshot.edges.push_back(std::move(item));
  }

  // Draw order follows the style's z value; a flat model has no other way to express it.
  std::stable_sort(snapshot.edges.begin(), snapshot.edges.end(), [](const EdgeItem& lhs, const EdgeItem& rhs) {
    return lhs.zValue < rhs.zValue;
  });

  auto left = std::numeric_limits<qreal>::max();
  auto top = std::numeric_limits<qreal>::max();
  auto right = std::numeric_limits<qreal>::lowest();
  auto bottom = std::numeric_limits<qreal>::lowest();
  for(const NodeItem& node : snapshot.nodes) {
    left = std::min(left, node.x);
    top = std::min(top, node.y);
    right = std::max(right, node.x + node.width);
    bottom = std::max(bottom, node.y + node.height);
    if(node.active && snapshot.activeTokenId == 0) {
      snapshot.activeTokenId = node.tokenId;
    }
  }
  if(!snapshot.nodes.empty()) {
    snapshot.boundsX = left;
    snapshot.boundsY = top;
    snapshot.boundsWidth = right - left;
    snapshot.boundsHeight = bottom - top;
  }

  mOnSnapshot(std::move(snapshot));
}

void QmlGraphView::clear() {
  mOnSnapshot(Snapshot{});
}

void QmlGraphView::coFocusTokenIds(const std::vector<Id>& /*focusedTokenIds*/) {}

void QmlGraphView::deCoFocusTokenIds(const std::vector<Id>& /*defocusedTokenIds*/) {}

void QmlGraphView::resizeView() {}

Vec2f QmlGraphView::getViewSize() const {
  return mViewSize;
}

GroupType QmlGraphView::getGrouping() const {
  return mGrouping;
}

void QmlGraphView::scrollToValues(int /*xValue*/, int /*yValue*/) {}

void QmlGraphView::activateEdge(Id /*edgeId*/) {}

void QmlGraphView::setNavigationFocus(bool focus) {
  mNavigationFocus = focus;
}

bool QmlGraphView::hasNavigationFocus() const {
  return mNavigationFocus;
}

bool QmlGraphView::isVisible() const {
  return true;
}

void QmlGraphView::findMatches(ScreenSearchSender* /*sender*/, const std::wstring& /*query*/) {}

void QmlGraphView::activateMatch(size_t /*matchIndex*/) {}

void QmlGraphView::deactivateMatch(size_t /*matchIndex*/) {}

void QmlGraphView::clearMatches() {}

}    // namespace graph
