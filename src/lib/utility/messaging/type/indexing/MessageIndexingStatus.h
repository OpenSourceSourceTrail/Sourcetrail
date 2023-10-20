#pragma once
#include "Message.h"

/**
 * @brief Message listeners what is the progress percent?
 *
 * @note It's forced to be a task
 */
struct MessageIndexingStatus final : Message<MessageIndexingStatus> {
  static std::string getStaticType() {
    return "MessageIndexingStatus";
  }

  explicit MessageIndexingStatus(bool showProgress_, size_t progressPercent_ = 0)
      : showProgress(showProgress_), progressPercent(progressPercent_) {
    setSendAsTask(false);
  }

  bool showProgress;
  size_t progressPercent;
};
