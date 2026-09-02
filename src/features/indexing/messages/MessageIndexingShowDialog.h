#pragma once
// internal
#include "Message.h"

class MessageIndexingShowDialog final : public Message<MessageIndexingShowDialog> {
public:
  static const std::string getStaticType() {
    return "MessageIndexingShowDialog";
  }

  MessageIndexingShowDialog() {
    setSendAsTask(false);
  }
};
