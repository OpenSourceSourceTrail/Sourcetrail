#pragma once
// internal
#include "Message.h"

class MessageRefreshUIState final : public Message<MessageRefreshUIState> {
public:
  static const std::string getStaticType() {
    return "MessageRefreshUIState";
  }

  MessageRefreshUIState(bool isAfterIndexing_) : isAfterIndexing(isAfterIndexing_) {}

  bool isAfterIndexing = false;
};