#pragma once
// interal
#include "Message.h"

class MessageIndexingStatus final : public Message<MessageIndexingStatus> {
public:
  static const std::string getStaticType() {
    return "MessageIndexingStatus";
  }

  MessageIndexingStatus(bool showProgress_, size_t progressPercent_ = 0)
      : showProgress(showProgress_), progressPercent(progressPercent_) {
    setSendAsTask(false);
  }

  const bool showProgress;
  const size_t progressPercent;
};