#pragma once
// internal
#include "Message.h"

/**
 * @brief Message listeners that the indexing interrupted
 *
 * @note It's forced to be a task.
 */
struct MessageIndexingInterrupted final : Message<MessageIndexingInterrupted> {
public:
  static std::string getStaticType() {
    return "MessageIndexingInterrupted";
  }

  MessageIndexingInterrupted() {
    setSendAsTask(false);
  }
};
