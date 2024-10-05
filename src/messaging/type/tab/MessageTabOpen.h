#pragma once
// internal
#include "Message.h"

class MessageTabOpen final : public Message<MessageTabOpen> {
public:
  static const std::string getStaticType() {
    return "MessageTabOpen";
  }
};