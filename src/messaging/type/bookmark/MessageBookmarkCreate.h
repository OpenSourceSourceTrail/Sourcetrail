#pragma once
// internal
#include "Message.h"

class MessageBookmarkCreate : public Message<MessageBookmarkCreate> {
public:
  MessageBookmarkCreate(Id nodeId_ = 0) : nodeId(nodeId_) {}

  static const std::string getStaticType() {
    return "MessageBookmarkCreate";
  }

  const Id nodeId;
};
