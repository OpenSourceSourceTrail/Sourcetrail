#pragma once
// internal
#include "component/TabId.h"
#include "Message.h"

class MessageShowError final : public Message<MessageShowError> {
public:
  static const std::string getStaticType() {
    return "MessageShowError";
  }

  MessageShowError(Id errorId_) : errorId(errorId_) {
    setSchedulerId(TabId::currentTab());
  }

  const Id errorId;
};