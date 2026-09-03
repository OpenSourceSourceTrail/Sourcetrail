#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageFind final : public Message<MessageFind> {
public:
  MessageFind(bool fulltext = false) : findFulltext(fulltext) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageFind";
  }

  bool findFulltext;
};