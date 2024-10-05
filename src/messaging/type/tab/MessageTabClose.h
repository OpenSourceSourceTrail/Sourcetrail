#pragma once
// internal
#include "Message.h"

class MessageTabClose final : public Message<MessageTabClose> {
public:
  static const std::string getStaticType() {
    return "MessageTabClose";
  }
};