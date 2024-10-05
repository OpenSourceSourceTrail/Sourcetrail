#pragma once
// internal
#include "Message.h"

class MessageErrorsHelpMessage final : public Message<MessageErrorsHelpMessage> {
public:
  static const std::string getStaticType() {
    return "MessageErrorsHelpMessage";
  }

  MessageErrorsHelpMessage(bool force_ = false) : force(force_) {}

  const bool force;
};