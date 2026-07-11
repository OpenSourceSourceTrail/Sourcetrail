#pragma once
// internal
#include "Message.h"

class MessagePingReceived final : public Message<MessagePingReceived> {
public:
  MessagePingReceived() = default;

  static const std::string getStaticType() {
    return "MessagePingReceived";
  }

  std::wstring ideName = {};
};