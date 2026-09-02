#pragma once
// internal
#include "Message.h"

class MessagePluginPortChange final : public Message<MessagePluginPortChange> {
public:
  MessagePluginPortChange() = default;

  static const std::string getStaticType() {
    return "MessagePluginPortChange";
  }
};