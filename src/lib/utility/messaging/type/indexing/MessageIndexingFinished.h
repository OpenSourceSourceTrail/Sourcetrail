#pragma once
#include "Message.h"

/**
 * @brief Message listeners that the indexing finished
 */
struct MessageIndexingFinished final : Message<MessageIndexingFinished> {
  static std::string getStaticType() {
    return "MessageIndexingFinished";
  }
};
