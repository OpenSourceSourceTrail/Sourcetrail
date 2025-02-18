#pragma once

#include "GlobalId.hpp"
#include "Message.h"
#include "NameHierarchy.h"
#include "TabId.h"

class MessageActivateNodes final : public Message<MessageActivateNodes> {
public:
  struct ActiveNode {
    ActiveNode() : nodeId(0), nameHierarchy(NAME_DELIMITER_UNKNOWN) {}
    Id nodeId;
    NameHierarchy nameHierarchy;
  };

  MessageActivateNodes(Id tokenId = 0) {
    if(tokenId > 0) {
      addNode(tokenId);
    }

    setSchedulerId(TabId::currentTab());
  }

  void addNode(Id tokenId) {
    ActiveNode node;
    node.nodeId = tokenId;
    nodes.push_back(node);
  }

  void addNode(Id tokenId, const NameHierarchy& nameHierarchy) {
    ActiveNode node;
    node.nodeId = tokenId;
    node.nameHierarchy = nameHierarchy;
    nodes.push_back(node);
  }

  static const std::string getStaticType() {
    return "MessageActivateNodes";
  }

  virtual void print(std::wostream& ostream) const {
    for(const ActiveNode& node : nodes) {
      ostream << node.nodeId << L" ";
    }
  }

  std::vector<ActiveNode> nodes;
};