#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageFocusChanged final : public Message<MessageFocusChanged> {
public:
  enum class ViewType { GRAPH, CODE };

  MessageFocusChanged(ViewType type_, Id tokenOrLocationId_) : type(type_), tokenOrLocationId(tokenOrLocationId_) {
    setIsLogged(false);
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageFocusChanged";
  }

  void print(std::wostream& ostream) const override {
    switch(type) {
    case ViewType::GRAPH:
      ostream << "graph";
      break;
    case ViewType::CODE:
      ostream << "code";
      break;
    }

    ostream << " " << tokenOrLocationId;
  }

  bool isFromGraph() const {
    return type == ViewType::GRAPH;
  }

  bool isFromCode() const {
    return type == ViewType::CODE;
  }

  const ViewType type;
  const Id tokenOrLocationId;
};