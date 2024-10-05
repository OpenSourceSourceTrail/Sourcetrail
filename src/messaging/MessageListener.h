#pragma once
// STL
#include <string>
// internal
#include "MessageBase.h"
#include "MessageListenerBase.h"

template <typename MessageType>
class MessageListener : public MessageListenerBase {
public:
  MessageListener() = default;

private:
  std::string doGetType() const override {
    return MessageType::getStaticType();
  }

  void doHandleMessageBase(MessageBase* pMessage) override {
    handleMessage(dynamic_cast<MessageType*>(pMessage));
  }

  virtual void handleMessage(MessageType* pMessage) = 0;
};