#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "types.h"

class MessageCodeShowDefinition final : public Message<MessageCodeShowDefinition> {
public:
  static const std::string getStaticType() {
    return "MessageCodeShowDefinition";
  }

  MessageCodeShowDefinition(Id nodeId_, bool inIDE_ = false) : nodeId(nodeId_), inIDE(inIDE_) {
    setSchedulerId(TabId::currentTab());
  }

  void print(std::wostream& ostream) const override {
    ostream << nodeId;
  }

  const Id nodeId;
  const bool inIDE;
};