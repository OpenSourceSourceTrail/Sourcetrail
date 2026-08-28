#pragma once
// internal
#include "data/graph/Edge.h"
#include "data/name/NameHierarchy.h"
//
#include "component/TabId.h"
#include "GlobalId.hpp"
#include "Message.h"

class MessageActivateEdge : public Message<MessageActivateEdge> {
public:
  MessageActivateEdge(Id tokenId_,
                      Edge::EdgeType type_,
                      const NameHierarchy& sourceNameHierarchy_,
                      const NameHierarchy& targetNameHierarchy_)
      : tokenId(tokenId_), type(type_), sourceNameHierarchy(sourceNameHierarchy_), targetNameHierarchy(targetNameHierarchy_) {
    if(!isBundledEdges()) {
      setKeepContent(true);
    }

    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateEdge";
  }

  bool isBundledEdges() const {
    return type == Edge::EDGE_BUNDLED_EDGES;
  }

  std::wstring getFullName() const {
    std::wstring name = Edge::getReadableTypeString(type) + L":";
    name += sourceNameHierarchy.getQualifiedNameWithSignature() + L"->";
    name += targetNameHierarchy.getQualifiedNameWithSignature();
    return name;
  }

  void print(std::wostream& ostream) const override {
    ostream << tokenId << L" - " << getFullName();
  }

  const Id tokenId;
  const Edge::EdgeType type;
  const NameHierarchy sourceNameHierarchy;
  const NameHierarchy targetNameHierarchy;

  std::vector<Id> bundledEdgesIds;
};