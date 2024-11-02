#pragma once
// STL
#include <list>
#include <vector>
// internal
#include "MessageListener.h"
#include "type/activation/MessageActivateErrors.h"
#include "type/activation/MessageActivateFullTextSearch.h"
#include "type/activation/MessageActivateLegend.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/activation/MessageActivateTokens.h"
#include "type/activation/MessageActivateTrail.h"
#include "type/code/MessageShowReference.h"
#include "type/focus/MessageFocusChanged.h"
#include "type/focus/MessageFocusIn.h"
#include "type/focus/MessageFocusOut.h"
#include "type/graph/MessageActivateTrailEdge.h"
#include "type/graph/MessageDeactivateEdge.h"
#include "type/graph/MessageGraphNodeBundleSplit.h"
#include "type/graph/MessageGraphNodeExpand.h"
#include "type/graph/MessageGraphNodeHide.h"
#include "type/graph/MessageGraphNodeMove.h"
#include "type/graph/MessageScrollGraph.h"
#include "type/MessageFlushUpdates.h"
//
#include "Controller.h"
#include "DummyEdge.h"
#include "DummyNode.h"
#include "GraphView.h"
#include "Node.h"

class Graph;
class StorageAccess;

class GraphController
    : public Controller
    , public MessageListener<MessageActivateErrors>
    , public MessageListener<MessageActivateFullTextSearch>
    , public MessageListener<MessageActivateLegend>
    , public MessageListener<MessageActivateOverview>
    , public MessageListener<MessageActivateTokens>
    , public MessageListener<MessageActivateTrail>
    , public MessageListener<MessageActivateTrailEdge>
    , public MessageListener<MessageDeactivateEdge>
    , public MessageListener<MessageFocusChanged>
    , public MessageListener<MessageFlushUpdates>
    , public MessageListener<MessageFocusIn>
    , public MessageListener<MessageFocusOut>
    , public MessageListener<MessageGraphNodeBundleSplit>
    , public MessageListener<MessageGraphNodeExpand>
    , public MessageListener<MessageGraphNodeHide>
    , public MessageListener<MessageGraphNodeMove>
    , public MessageListener<MessageScrollGraph>
    , public MessageListener<MessageShowReference> {
public:
  GraphController(StorageAccess* storageAccess);
  ~GraphController() override;

  Id getSchedulerId() const override;

private:
  void handleMessage(MessageActivateErrors* message) override;
  void handleMessage(MessageActivateFullTextSearch* message) override;
  void handleMessage(MessageActivateLegend* message) override;
  void handleMessage(MessageActivateOverview* message) override;
  void handleMessage(MessageActivateTokens* message) override;
  void handleMessage(MessageActivateTrail* message) override;
  void handleMessage(MessageActivateTrailEdge* message) override;
  void handleMessage(MessageDeactivateEdge* message) override;
  void handleMessage(MessageFocusChanged* message) override;
  void handleMessage(MessageFlushUpdates* message) override;
  void handleMessage(MessageFocusIn* message) override;
  void handleMessage(MessageFocusOut* message) override;
  void handleMessage(MessageGraphNodeBundleSplit* message) override;
  void handleMessage(MessageGraphNodeExpand* message) override;
  void handleMessage(MessageGraphNodeHide* message) override;
  void handleMessage(MessageGraphNodeMove* message) override;
  void handleMessage(MessageScrollGraph* message) override;
  void handleMessage(MessageShowReference* message) override;

  GraphView* getView() const;

  void clear() override;

  void createDummyGraph(const std::shared_ptr<Graph> graph);
  void createDummyGraphAndSetActiveAndVisibility(const std::vector<Id>& tokenIds,
                                                 const std::shared_ptr<Graph> graph,
                                                 bool keepExpandedNodesExpanded);
  std::vector<std::shared_ptr<DummyNode>> createDummyNodeTopDown(Node* node, Id ancestorId);

  void updateDummyNodeNamesAndAddQualifiers(const std::vector<std::shared_ptr<DummyNode>>& dummyNodes);

  std::vector<Id> getExpandedNodeIds() const;
  void setExpandedNodeIds(const std::vector<Id>& nodeIds);
  void autoExpandActiveNode(const std::vector<Id>& activeTokenIds);

  bool setActive(const std::vector<Id>& activeTokenIds, bool showAllEdges);
  void setVisibility(bool noActive);
  void setActiveAndVisibility(const std::vector<Id>& activeTokenIds);
  bool setNodeActiveRecursive(DummyNode* node, const std::vector<Id>& activeTokenIds) const;
  bool setNodeVisibilityRecursiveBottomUp(DummyNode* node, bool noActive) const;
  void setNodeVisibilityRecursiveTopDown(DummyNode* node, bool parentExpanded) const;

  void hideBuiltinTypes();

  void bundleNodes();
  void bundleNodesAndEdgesMatching(std::function<bool(const DummyNode::BundleInfo&, const Node*)> matcher,
                                   size_t count,
                                   bool countConnectedNodes,
                                   const std::wstring& name);
  std::shared_ptr<DummyNode> bundleNodesMatching(std::list<std::shared_ptr<DummyNode>>& nodes,
                                                 std::function<bool(const DummyNode*)> matcher,
                                                 const std::wstring& name);
  std::shared_ptr<DummyNode> bundleByType(std::list<std::shared_ptr<DummyNode>>& nodes,
                                          const NodeType& type,
                                          const Tree<NodeType::BundleInfo>& bundleInfoTree,
                                          const bool considerInvisibleNodes);
  void bundleNodesByType();

  void addCharacterIndex();
  bool hasCharacterIndex() const;

  void groupNodesByParents(GroupType groupType);
  DummyNode* groupAllNodes(GroupType groupType, Id groupNodeId);
  void groupTrailNodes(GroupType groupType);

  void layoutNesting();
  void extendEqualFunctionNames(const std::vector<std::shared_ptr<DummyNode>>& nodes) const;
  QVector4D layoutNestingRecursive(DummyNode* node, int relayoutAccessMaxWidth = -1) const;
  void addExpandToggleNode(DummyNode* node) const;
  void layoutToGrid(DummyNode* node) const;

  void layoutGraph(bool getSortedNodes = false);
  void layoutList();
  void layoutTrail(bool horizontal, bool hasOrigin);

  void assignBundleIds();

  std::shared_ptr<DummyNode> getDummyGraphNodeById(Id tokenId) const;
  DummyEdge* getDummyGraphEdgeById(Id tokenId) const;

  void relayoutGraph(MessageBase* message, GraphView::GraphParams params, bool withCharacterIndex, const std::wstring& groupName);
  void buildGraph(MessageBase* pMessage, GraphView::GraphParams params);

  void forEachDummyNodeRecursive(std::function<void(DummyNode*)> func);
  void forEachDummyEdge(std::function<void(DummyEdge*)> func);

  void createLegendGraph();

  StorageAccess* m_storageAccess;

  std::vector<std::shared_ptr<DummyNode>> m_dummyNodes;
  std::vector<std::shared_ptr<DummyEdge>> m_dummyEdges;

  std::map<Id, std::shared_ptr<DummyNode>> m_dummyGraphNodes;

  std::vector<Id> m_activeNodeIds;
  std::vector<Id> m_activeEdgeIds;

  std::shared_ptr<Graph> m_graph;

  std::map<Id, Id> m_topLevelAncestorIds;

  bool m_useBezierEdges = false;
  bool m_showsLegend = false;
  Id m_tokenIdToFocus = 0;
};