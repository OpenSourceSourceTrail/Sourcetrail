#pragma once
// internal
#include "component/TabId.h"
#include "Message.h"

class MessageDeactivateEdge final : public Message<MessageDeactivateEdge> {
public:
  MessageDeactivateEdge(bool scrollToDefinition_) : scrollToDefinition(scrollToDefinition_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageDeactivateEdge";
  }

  bool scrollToDefinition;
};