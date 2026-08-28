#ifndef MESSAGE_HISTORY_UNDO_H
#define MESSAGE_HISTORY_UNDO_H

#include "component/TabId.h"
#include "Message.h"

class MessageHistoryUndo : public Message<MessageHistoryUndo> {
public:
  static const std::string getStaticType() {
    return "MessageHistoryUndo";
  }

  MessageHistoryUndo() {
    setSchedulerId(TabId::currentTab());
  }
};

#endif    // MESSAGE_HISTORY_UNDO_H
