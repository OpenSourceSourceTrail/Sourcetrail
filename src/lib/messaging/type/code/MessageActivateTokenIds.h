#pragma once
// internal
#include "GlobalId.hpp"
#include "Message.h"
#include "TabId.h"

class MessageActivateTokenIds final : public Message<MessageActivateTokenIds> {
public:
  MessageActivateTokenIds(const std::vector<Id>& tokenIds_) : tokenIds(tokenIds_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateTokenIds";
  }

  void print(std::wostream& ostream) const override {
    for(const Id& id : tokenIds) {
      ostream << id << L" ";
    }
  }

  const std::vector<Id> tokenIds;
};