#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "Vector2.h"
#include "types.h"

class MessageGraphNodeMove final : public Message<MessageGraphNodeMove> {
public:
  MessageGraphNodeMove(Id tokenId_, const Vec2i& delta_) : tokenId(tokenId_), delta(delta_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageGraphNodeMove";
  }

  void print(std::wostream& ostream) const override {
    ostream << tokenId << L" " << delta.toWString();
  }

  const Id tokenId;
  const Vec2i delta;
};
