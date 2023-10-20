#pragma once
#include "Message.h"

/**
 * @brief Message listeners that the indexing started
 */
struct MessageIndexingStarted final : Message<MessageIndexingStarted> {
  static std::string getStaticType() {
    return "MessageIndexingStarted";
  }
};
