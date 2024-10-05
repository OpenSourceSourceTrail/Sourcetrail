#pragma once
// internal
#include "Message.h"

class MessageCustomTrailShow final : public Message<MessageCustomTrailShow> {
public:
  MessageCustomTrailShow() = default;

  static const std::string getStaticType() {
    return "MessageCustomTrailShow";
  }
};