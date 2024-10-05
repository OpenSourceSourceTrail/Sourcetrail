#pragma once
// internal
#include "Message.h"

class MessageErrorsAll final : public Message<MessageErrorsAll> {
public:
  static const std::string getStaticType() {
    return "MessageErrorsAll";
  }

  MessageErrorsAll() = default;
};