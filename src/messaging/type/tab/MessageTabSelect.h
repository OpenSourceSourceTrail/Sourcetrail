#pragma once
// internal
#include "Message.h"

class MessageTabSelect final : public Message<MessageTabSelect> {
public:
  MessageTabSelect(bool next_) : next(next_) {}

  static const std::string getStaticType() {
    return "MessageTabSelect";
  }

  bool next;
};