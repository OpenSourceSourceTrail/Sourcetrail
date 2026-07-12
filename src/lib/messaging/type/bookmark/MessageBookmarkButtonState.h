#pragma once
// internal
#include "Message.h"

class MessageBookmarkButtonState : public Message<MessageBookmarkButtonState> {
public:
  enum ButtonState { CAN_CREATE = 0, CANNOT_CREATE, ALREADY_CREATED };

  static const std::string getStaticType() {
    return "MessageBookmarkButtonState";
  }

  MessageBookmarkButtonState(Id schedulerId, ButtonState state_) : state(state_) {
    setSchedulerId(schedulerId);
  }

  const ButtonState state;
};