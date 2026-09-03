#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageFocusView final : public Message<MessageFocusView> {
public:
  enum class ViewType { GRAPH, CODE, TOGGLE };

  MessageFocusView(ViewType type_) : type(type_) {
    setIsLogged(false);
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageFocusView";
  }

  void print(std::wostream& ostream) const override {
    switch(type) {
    case ViewType::GRAPH:
      ostream << "graph";
      break;
    case ViewType::CODE:
      ostream << "code";
      break;
    case ViewType::TOGGLE:
      ostream << "toggle";
      break;
    }
  }

  const ViewType type;
};