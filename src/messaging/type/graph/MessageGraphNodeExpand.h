#pragma once

#include "Message.h"
#include "TabId.h"
#include "GlobalId.hpp"

class MessageGraphNodeExpand final : public Message<MessageGraphNodeExpand> {
public:
  MessageGraphNodeExpand(Id tokenId_, bool expand_, bool ignoreIfNotReplayed_ = false)
      : tokenId(tokenId_), expand(expand_), ignoreIfNotReplayed(ignoreIfNotReplayed_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageGraphNodeExpand";
  }

  void print(std::wostream& ostream) const override {
    ostream << tokenId << L" ";
    if(expand) {
      ostream << L"expand";
    } else {
      ostream << L"collapse";
    }
  }

  const Id tokenId;
  const bool expand;
  const bool ignoreIfNotReplayed;
};