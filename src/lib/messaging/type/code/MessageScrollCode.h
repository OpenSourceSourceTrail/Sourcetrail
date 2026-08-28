#pragma once
// internal
#include "component/TabId.h"
#include "Message.h"

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
