#pragma once
// internal
#include "Message.h"

class MessageErrorCountClear final : public Message<MessageErrorCountClear> {
public:
  static const std::string getStaticType() {
    return "MessageErrorCountClear";
  }

  MessageErrorCountClear() {
    setSendAsTask(false);
  }
};