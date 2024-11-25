#pragma once
// internal
#include "Edge.h"
#include "Message.h"
#include "NameHierarchy.h"
#include "TabId.h"
#include "GlobalId.hpp"

class MessageActivateTrailEdge final : public Message<MessageActivateTrailEdge> {
public:
  MessageActivateTrailEdge(const std::vector<Id>& edgeIds_,
                           Edge::EdgeType type_,
                           const NameHierarchy& sourceNameHierarchy_,
                           const NameHierarchy& targetNameHierarchy_)
      : edgeIds(edgeIds_), type(type_), sourceNameHierarchy(sourceNameHierarchy_), targetNameHierarchy(targetNameHierarchy_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateTrailEdge";
  }

  std::wstring getFullName() const {
    std::wstring name = Edge::getReadableTypeString(type) + L":";
    name += sourceNameHierarchy.getQualifiedNameWithSignature() + L"->";
    name += targetNameHierarchy.getQualifiedNameWithSignature();
    return name;
  }

  void print(std::wostream& ostream) const override {
    for(Id edgeId : edgeIds) {
      ostream << edgeId << L",";
    }
    ostream << L" - " << getFullName();
  }

  const std::vector<Id> edgeIds;
  const Edge::EdgeType type;
  const NameHierarchy sourceNameHierarchy;
  const NameHierarchy targetNameHierarchy;
};