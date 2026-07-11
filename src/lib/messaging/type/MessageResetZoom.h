#pragma once
#include "Message.h"

class MessageResetZoom : public Message<MessageResetZoom> {
public:
  static std::string getStaticType() {
    return "MessageResetZoom";
  }
};
