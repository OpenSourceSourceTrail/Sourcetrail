#pragma once
// internal
#include "Message.h"

class MessageScrollSpeedChange : public Message<MessageScrollSpeedChange> {
public:
  MessageScrollSpeedChange(float scrollSpeed_) : scrollSpeed(scrollSpeed_) {}

  static const std::string getStaticType() {
    return "MessageScrollSpeedChange";
  }

  virtual void print(std::wostream& ostream) const {
    ostream << scrollSpeed;
  }

  const float scrollSpeed;
};