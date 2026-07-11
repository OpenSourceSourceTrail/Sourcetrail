#pragma once
// internal
#include "Message.h"
#include "NodeTypeSet.h"
#include "TabId.h"

class MessageSearchAutocomplete final : public Message<MessageSearchAutocomplete> {
public:
  MessageSearchAutocomplete(std::wstring query_, NodeTypeSet acceptedNodeTypes_)
      : query(std::move(query_)), acceptedNodeTypes(acceptedNodeTypes_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageSearchAutocomplete";
  }

  void print(std::wostream& ostream) const override {
    ostream << query << L"[";
    std::vector<Id> nodeTypeIds = acceptedNodeTypes.getNodeTypeIds();
    for(size_t i = 0; i < nodeTypeIds.size(); i++) {
      if(i != 0) {
        ostream << L", ";
      }
      ostream << std::to_wstring(nodeTypeIds[i]);
    }
    ostream << L"]";
  }

  const std::wstring query;
  const NodeTypeSet acceptedNodeTypes;
};