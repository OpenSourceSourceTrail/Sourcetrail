#ifndef MESSAGE_HISTORY_REDO_H
#define MESSAGE_HISTORY_REDO_H

#include "component/TabId.h"
#include "Message.h"

class MessageHistoryRedo : public Message<MessageHistoryRedo> {
public:
  static const std::string getStaticType() {
    return "MessageHistoryRedo";
  }

  MessageHistoryRedo() {
    setSchedulerId(TabId::currentTab());
  }
};

#endif    // MESSAGE_HISTORY_REDO_H
