#pragma once
// internal
#include "Message.h"

/**
 * @brief Message listeners to show dialog
 *
 * @note It's forced to be a task.
 */
class MessageIndexingShowDialog final : public Message<MessageIndexingShowDialog> {
public:
  static std::string getStaticType() {
    return "MessageIndexingShowDialog";
  }

  MessageIndexingShowDialog() {
    setSendAsTask(false);
  }
};
