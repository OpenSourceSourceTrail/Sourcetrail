#pragma once
#include "ErrorCountInfo.h"
#include "ErrorInfo.h"
#include "Message.h"

class MessageErrorCountUpdate final : public Message<MessageErrorCountUpdate> {
public:
  static std::string getStaticType() {
    return "MessageErrorCountUpdate";
  }

  MessageErrorCountUpdate(const ErrorCountInfo& errorCount_, const std::vector<ErrorInfo>& newErrors_)
      : errorCount(errorCount_), newErrors(newErrors_) {
    setSendAsTask(false);
  }

  void print(std::wostream& ostream) const override {
    ostream << errorCount.total << '/' << errorCount.fatal << L" - " << newErrors.size() << L" new errors";
  }

  const ErrorCountInfo errorCount;
  std::vector<ErrorInfo> newErrors;
};
