#pragma once
// internal
#include "Message.h"

class MessageWindowFocus : public Message<MessageWindowFocus> {
public:
  MessageWindowFocus(bool focusIn_) : focusIn(focusIn_) {}

  static const std::string getStaticType() {
    return "MessageWindowFocus";
  }

  const bool focusIn;
};
