#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "types.h"

class MessageGraphNodeHide final : public Message<MessageGraphNodeHide> {
public:
  MessageGraphNodeHide(Id tokenId_) : tokenId(tokenId_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageGraphNodeHide";
  }

  void print(std::wostream& ostream) const override {
    ostream << tokenId;
  }

  const Id tokenId;
};
