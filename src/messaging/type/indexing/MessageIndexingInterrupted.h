#pragma once
// internal
#include "Message.h"

class MessageIndexingInterrupted final : public Message<MessageIndexingInterrupted> {
public:
  static const std::string getStaticType() {
    return "MessageIndexingInterrupted";
  }

  MessageIndexingInterrupted() {
    setSendAsTask(false);
  }
};