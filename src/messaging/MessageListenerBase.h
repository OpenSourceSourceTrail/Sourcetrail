#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"
#include "MessageBase.h"
#include "MessageQueue.h"

class MessageListenerBase {
public:
  MessageListenerBase() : m_id(s_nextId++), m_alive(true) {
    IMessageQueue::getInstance()->registerListener(this);
  }

  virtual ~MessageListenerBase();

  Id getId() const {
    return m_id;
  }

  std::string getType() const {
    return m_alive ? doGetType() : "";
  }

  void handleMessageBase(MessageBase* pMessage) {
    if(m_alive) {
      doHandleMessageBase(pMessage);
    }
  }

  void removedListener() {
    m_alive = false;
  }

  virtual Id getSchedulerId() const {
    return 0;
  }

private:
  virtual std::string doGetType() const = 0;
  virtual void doHandleMessageBase(MessageBase*) = 0;

  static Id s_nextId;

  Id m_id;
  bool m_alive;
};