#pragma once
// internal
#include "component/TabId.h"
#include "Message.h"

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