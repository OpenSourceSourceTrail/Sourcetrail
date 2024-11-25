#pragma once
// STL
#include <vector>
// internal
#include "Message.h"
#include "TabId.h"
#include "GlobalId.hpp"

class MessageFocusOut final : public Message<MessageFocusOut> {
public:
  MessageFocusOut(const std::vector<Id>& tokenIds_) : tokenIds(tokenIds_) {
    setIsLogged(false);
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageFocusOut";
  }

  void print(std::wostream& ostream) const override {
    for(const Id& id : tokenIds) {
      ostream << id << L" ";
    }
  }

  const std::vector<Id> tokenIds;
};