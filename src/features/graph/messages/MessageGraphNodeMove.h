#pragma once

#include <fmt/xchar.h>

#include "Vec2f.h"

// internal
#include "GlobalId.hpp"
#include "Message.h"
#include "TabId.h"

class MessageGraphNodeMove final : public Message<MessageGraphNodeMove> {
public:
  MessageGraphNodeMove(Id tokenId_, const Vec2f& delta_) : tokenId(tokenId_), delta(delta_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageGraphNodeMove";
  }

  void print(std::wostream& ostream) const override {
    ostream << tokenId << L" " << fmt::format(L"[{}, {}]", delta.x, delta.y);
  }

  const Id tokenId;
  const Vec2f delta;
};
