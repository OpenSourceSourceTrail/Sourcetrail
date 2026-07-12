#pragma once
// internal
#include "Message.h"

class MessageIndexingFinished final : public Message<MessageIndexingFinished> {
public:
  static const std::string getStaticType() {
    return "MessageIndexingFinished";
  }

  MessageIndexingFinished() = default;
};