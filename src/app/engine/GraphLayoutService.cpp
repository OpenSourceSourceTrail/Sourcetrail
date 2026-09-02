#include "GraphLayoutService.h"

#include "activation/messages/MessageActivateTokens.h"
#include "component/Component.h"
#include "component/controller/GraphController.h"
#include "component/controller/helper/DummyEdge.h"
#include "component/controller/helper/DummyNode.h"
#include "component/view/GraphView.h"
#include "component/view/GraphViewStyle.h"
#include "component/view/MetricGraphViewStyleImpl.h"
#include "data/graph/Edge.h"
#include "data/graph/Graph.h"
#include "data/graph/Node.h"
#include "engine.pb.h"
#include "logging.h"
#include "MessageQueue.h"
#include "utilityString.h"

namespace {

/**
 * GraphViewStyle::loadStyleSettings() reads this as the base size the measured metrics belong to.
 */
constexpr size_t ReferenceFontSize = 14;

/** A viewport big enough that BucketLayouter still produces a sane grid when a client sends none. */
constexpr float FallbackViewWidth = 1200.0F;
constexpr float FallbackViewHeight = 800.0F;

/**
 * MessageActivateTokens copies keepContent() and the scheduler id from whatever message caused it.
 * An HTTP request is caused by no message, so it copies from this neutral stand-in.
 */
struct OriginMessage final : MessageBase {
  [[nodiscard]] std::string getType() const override {
    return "GraphLayoutRequest";
  }
  void dispatch() override {}
  void print(std::wostream&) const override {}
};

GroupType groupingFromInt(int value) {
  switch(value) {
  case 1:
    return GroupType::FILE_TYPE;
  case 2:
    return GroupType::NAMESPACE;
  default:
    return GroupType::NONE;
  }
}

}    // namespace

/**
 * A GraphView that draws nothing and only remembers what it was handed.
 *
 * Everything GraphController asks a view for is either a layout input (viewport, grouping) or a
 * notification a headless run has no use for.
 */
class HeadlessGraphView : public GraphView {
public:
  HeadlessGraphView() : GraphView(nullptr) {}

  void createWidgetWrapper() override {}
  void refreshView() override {}

  void rebuildGraph(std::shared_ptr<Graph> graph,
                    const std::vector<std::shared_ptr<DummyNode>>& nodes,
                    const std::vector<std::shared_ptr<DummyEdge>>& edges,
                    const GraphParams /*params*/) override {
    mGraph = std::move(graph);
    mNodes = nodes;
    mEdges = edges;
  }

  void clear() override {
    mGraph.reset();
    mNodes.clear();
    mEdges.clear();
  }

  void coFocusTokenIds(const std::vector<Id>&) override {}
  void deCoFocusTokenIds(const std::vector<Id>&) override {}
  void resizeView() override {}

  [[nodiscard]] Vec2f getViewSize() const override {
    return mViewSize;
  }

  [[nodiscard]] GroupType getGrouping() const override {
    return mGrouping;
  }

  void scrollToValues(int, int) override {}
  void activateEdge(Id) override {}
  void setNavigationFocus(bool) override {}

  [[nodiscard]] bool hasNavigationFocus() const override {
    return false;
  }

  // ScreenSearchResponder -- no screen search without a screen.
  [[nodiscard]] bool isVisible() const override {
    return false;
  }
  void findMatches(ScreenSearchSender*, const std::wstring&) override {}
  void activateMatch(size_t) override {}
  void deactivateMatch(size_t) override {}
  void clearMatches() override {}

  void setLayoutInputs(Vec2f viewSize, GroupType grouping) {
    mViewSize = viewSize;
    mGrouping = grouping;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<DummyNode>>& nodes() const {
    return mNodes;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<DummyEdge>>& edges() const {
    return mEdges;
  }

private:
  Vec2f mViewSize{FallbackViewWidth, FallbackViewHeight};
  GroupType mGrouping = GroupType::NONE;

  std::shared_ptr<Graph> mGraph;
  std::vector<std::shared_ptr<DummyNode>> mNodes;
  std::vector<std::shared_ptr<DummyEdge>> mEdges;
};

namespace {

void toProto(const DummyNode& node, sourcetrail::LayoutNode* out) {
  out->set_dummy_type(static_cast<int>(node.type));
  out->set_token_id(node.tokenId);
  out->set_name(utility::encodeToUtf8(node.name));
  out->set_x(node.position.x);
  out->set_y(node.position.y);
  out->set_width(node.size.x);
  out->set_height(node.size.y);
  out->set_visible(node.visible);
  out->set_active(node.active);
  out->set_connected(node.connected);
  out->set_expanded(node.expanded);
  out->set_access_kind(static_cast<int>(node.accessKind));
  out->set_node_kind(node.data != nullptr ? nodeKindToInt(node.data->getType().getKind()) : 0);
  out->set_bundled_node_count(node.isBundleNode() ? node.getBundledNodeCount() : 0);
  out->set_has_missing_child_nodes(node.isGraphNode() && node.hasMissingChildNodes());

  for(const std::shared_ptr<DummyNode>& subNode : node.subNodes) {
    // Invisible nodes are layout scaffolding; they carry no position and nothing paints them.
    if(subNode->visible) {
      toProto(*subNode, out->add_sub_nodes());
    }
  }
}

void toProto(const DummyEdge& edge, sourcetrail::LayoutEdge* out) {
  out->set_token_id(edge.data != nullptr ? edge.data->getId() : 0);
  out->set_owner_id(edge.ownerId);
  out->set_target_id(edge.targetId);
  out->set_edge_kind(edge.data != nullptr ? Edge::typeToInt(edge.data->getType()) : 0);
  out->set_visible(edge.visible);
  out->set_active(edge.active);
  out->set_weight(edge.getWeight());
  out->set_direction(static_cast<int>(edge.getDirection()));
}

}    // namespace

GraphLayoutService::GraphLayoutService(StorageAccess* storageAccess) : mStorageAccess(storageAccess) {}

bool GraphLayoutService::ensureController() {
  if(mComponent) {
    return true;
  }
  // GraphController registers with IMessageQueue on construction; without one that is a null
  // dereference rather than a degraded layout.
  if(!IMessageQueue::hasInstance()) {
    LOG_ERROR("Cannot lay out a graph before the message queue exists.");
    return false;
  }
  mView = std::make_shared<HeadlessGraphView>();
  mController = std::make_shared<GraphController>(mStorageAccess);
  mComponent = std::make_unique<Component>(mView, mController);
  return true;
}

GraphLayoutService::~GraphLayoutService() = default;

sourcetrail::GraphLayoutResponse GraphLayoutService::layout(const sourcetrail::GraphLayoutRequest& request) {
  const std::lock_guard<std::mutex> lock(mMutex);

  sourcetrail::GraphLayoutResponse response;
  if(!ensureController()) {
    return response;
  }

  const float charWidth = request.char_width();
  const float charHeight = request.char_height();
  if(charWidth <= 0.0F || charHeight <= 0.0F) {
    // Without metrics every node box would be zero-width, which reads as a laid-out empty graph
    // rather than as the missing input it is.
    return response;
  }

  GraphViewStyle::setImpl(std::make_shared<MetricGraphViewStyleImpl>(charWidth, charHeight, ReferenceFontSize));
  // Must follow setImpl: it clears the cached per-style-type widths derived from the old metrics.
  GraphViewStyle::loadStyleSettings();

  mView->setLayoutInputs({request.view_width() > 0.0F ? request.view_width() : FallbackViewWidth,
                          request.view_height() > 0.0F ? request.view_height() : FallbackViewHeight},
                         groupingFromInt(request.grouping()));

  // Dispatching through IMessageQueue would hand this to the queue's thread and return before the
  // layout existed. The controller is ours alone, so it is called directly instead.
  const OriginMessage origin;
  MessageActivateTokens message(&origin);
  message.tokenIds.assign(request.token_ids().begin(), request.token_ids().end());
  static_cast<MessageListener<MessageActivateTokens>*>(mController.get())->handleMessageBase(&message);

  for(const std::shared_ptr<DummyNode>& node : mView->nodes()) {
    if(node->visible) {
      toProto(*node, response.add_nodes());
    }
  }
  for(const std::shared_ptr<DummyEdge>& edge : mView->edges()) {
    if(edge->visible) {
      toProto(*edge, response.add_edges());
    }
  }
  return response;
}
