#include "ConvertGraph.h"

#include "data/graph/Edge.h"
#include "data/graph/Graph.h"
#include "data/graph/Node.h"
#include "data/graph/token_component/TokenComponentAccess.h"
#include "data/graph/token_component/TokenComponentBundledEdges.h"
#include "data/graph/token_component/TokenComponentFilePath.h"
#include "data/graph/token_component/TokenComponentInheritanceChain.h"
#include "data/graph/token_component/TokenComponentIsAmbiguous.h"
#include "data/name/NameHierarchy.h"
#include "data/NodeType.h"
#include "utilityString.h"

namespace {

void nodeToProto(Node* node, sourcetrail::ProtoGraph::ProtoNode* msg) {
  msg->set_id(node->getId());
  msg->set_node_kind(nodeKindToInt(node->getType().getKind()));
  msg->set_name_hierarchy_serialized(utility::encodeToUtf8(NameHierarchy::serialize(node->getNameHierarchy())));
  msg->set_definition_kind(definitionKindToInt(node->isImplicit()     ? DEFINITION_IMPLICIT :
                                                   node->isExplicit() ? DEFINITION_EXPLICIT :
                                                                        DEFINITION_NONE));
  msg->set_child_count(node->getChildCount());

  if(const auto* access = node->getComponent<TokenComponentAccess>(); access != nullptr) {
    msg->set_access_kind(accessKindToInt(access->getAccess()));
  }

  if(const auto* filePath = node->getComponent<TokenComponentFilePath>(); filePath != nullptr) {
    msg->set_file_path(utility::encodeToUtf8(filePath->getFilePath().wstr()));
    msg->set_file_path_complete(filePath->isComplete());
  }
}

void edgeToProto(Edge* edge, sourcetrail::ProtoGraph::ProtoEdge* msg) {
  msg->set_id(edge->getId());
  msg->set_type(Edge::typeToInt(edge->getType()));
  msg->set_from_node_id(edge->getFrom()->getId());
  msg->set_to_node_id(edge->getTo()->getId());

  msg->set_is_ambiguous(edge->getComponent<TokenComponentIsAmbiguous>() != nullptr);

  if(const auto* chain = edge->getComponent<TokenComponentInheritanceChain>(); chain != nullptr) {
    for(const Id edgeId : chain->inheritanceEdgeIds) {
      msg->add_inheritance_chain_edge_ids(edgeId);
    }
  }

  if(const auto* bundled = edge->getComponent<TokenComponentBundledEdges>(); bundled != nullptr) {
    msg->set_has_bundled_edges(true);
    for(const auto& [bundledId, direction] : bundled->getBundledEdgesWithDirection()) {
      auto* entry = msg->add_bundled_edges();
      entry->set_id(bundledId);
      entry->set_forward(direction == TokenComponentBundledEdges::DIRECTION_FORWARD);
    }
  }
}

}    // namespace

namespace proto::convert {

sourcetrail::ProtoGraph toProto(const Graph& graph) {
  sourcetrail::ProtoGraph msg;

  graph.forEachNode([&msg](Node* node) { nodeToProto(node, msg.add_nodes()); });
  graph.forEachEdge([&msg](Edge* edge) { edgeToProto(edge, msg.add_edges()); });

  msg.set_trail_mode(static_cast<int>(graph.getTrailMode()));
  msg.set_has_trail_origin(graph.hasTrailOrigin());

  return msg;
}

std::shared_ptr<Graph> fromProto(const sourcetrail::ProtoGraph& msg) {
  auto graph = std::make_shared<Graph>();

  // Nodes first: Edge's constructor needs both endpoint Node pointers, and Node's NameHierarchy is
  // const so it must be supplied at construction time.
  for(const auto& nodeMsg : msg.nodes()) {
    Node* node = graph->createNode(nodeMsg.id(),
                                   NodeType(intToNodeKind(nodeMsg.node_kind())),
                                   NameHierarchy::deserialize(utility::decodeFromUtf8(nodeMsg.name_hierarchy_serialized())),
                                   intToDefinitionKind(nodeMsg.definition_kind()));
    if(node == nullptr) {
      continue;
    }
    node->setChildCount(nodeMsg.child_count());

    if(nodeMsg.has_access_kind()) {
      node->addComponent(std::make_shared<TokenComponentAccess>(intToAccessKind(nodeMsg.access_kind())));
    }
    if(nodeMsg.has_file_path()) {
      node->addComponent(std::make_shared<TokenComponentFilePath>(
          FilePath(utility::decodeFromUtf8(nodeMsg.file_path())), nodeMsg.file_path_complete()));
    }
  }

  for(const auto& edgeMsg : msg.edges()) {
    Node* from = graph->getNodeById(edgeMsg.from_node_id());
    Node* to = graph->getNodeById(edgeMsg.to_node_id());
    if(from == nullptr || to == nullptr) {
      // A truncated or inconsistent message must not produce a half-built edge.
      continue;
    }

    Edge* edge = graph->createEdge(edgeMsg.id(), Edge::intToType(edgeMsg.type()), from, to);
    if(edge == nullptr) {
      continue;
    }

    if(edgeMsg.is_ambiguous()) {
      edge->addComponent(std::make_shared<TokenComponentIsAmbiguous>());
    }

    if(edgeMsg.inheritance_chain_edge_ids_size() > 0) {
      std::vector<Id> chain;
      chain.reserve(static_cast<size_t>(edgeMsg.inheritance_chain_edge_ids_size()));
      for(const auto edgeId : edgeMsg.inheritance_chain_edge_ids()) {
        chain.push_back(edgeId);
      }
      edge->addComponent(std::make_shared<TokenComponentInheritanceChain>(std::move(chain)));
    }

    if(edgeMsg.has_bundled_edges()) {
      auto bundled = std::make_shared<TokenComponentBundledEdges>();
      for(const auto& entry : edgeMsg.bundled_edges()) {
        bundled->addBundledEdgesId(entry.id(), entry.forward());
      }
      edge->addComponent(std::move(bundled));
    }
  }

  graph->setTrailMode(static_cast<Graph::TrailMode>(msg.trail_mode()));
  graph->setHasTrailOrigin(msg.has_trail_origin());

  return graph;
}

}    // namespace proto::convert
