#pragma once
// internal
#include "Message.h"
#include "MessageActivateBase.h"
//
#include "NodeTypeSet.h"
#include "TabId.h"

class MessageActivateOverview
    : public Message<MessageActivateOverview>
    , public MessageActivateBase {
public:
  static const std::string getStaticType() {
    return "MessageActivateOverview";
  }

  MessageActivateOverview(NodeTypeSet acceptedNodeTypes_ = NodeTypeSet::all()) : acceptedNodeTypes(acceptedNodeTypes_) {
    setIsParallel(true);
    setSchedulerId(TabId::currentTab());
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    if(acceptedNodeTypes != NodeTypeSet::all()) {
      return SearchMatch::createCommandsForNodeTypes(acceptedNodeTypes);
    }
    return {SearchMatch::createCommand(SearchMatch::COMMAND_ALL)};
  }

  NodeTypeSet acceptedNodeTypes;
};
