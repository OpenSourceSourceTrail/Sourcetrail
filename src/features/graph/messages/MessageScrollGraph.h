#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageScrollGraph final : public Message<MessageScrollGraph> {
public:
  MessageScrollGraph(int xValue_, int yValue_) : xValue(xValue_), yValue(yValue_) {
    setIsLogged(false);
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageScrollGraph";
  }

  int xValue;
  int yValue;
};
