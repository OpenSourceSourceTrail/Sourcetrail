#pragma once
// internal
#include "Message.h"

class MessageIndexingStarted final : public Message<MessageIndexingStarted> {
public:
  static const std::string getStaticType() {
    return "MessageIndexingStarted";
  }

  MessageIndexingStarted() = default;
};