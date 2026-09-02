#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageScrollCode final : public Message<MessageScrollCode> {
public:
  MessageScrollCode(int value_, bool inListMode_) : value(value_), inListMode(inListMode_) {
    setIsLogged(false);
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageScrollCode";
  }

  int value;
  bool inListMode;
};
