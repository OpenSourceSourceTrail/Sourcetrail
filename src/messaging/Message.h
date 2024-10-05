#pragma once
// STL
#include <memory>
#include <string>
// internal
#include "MessageBase.h"
#include "MessageQueue.h"

template <typename MessageType>
class Message : public MessageBase {
public:
  ~Message() override = default;

  [[nodiscard]] std::string getType() const override {
    return MessageType::getStaticType();
  }

  void dispatch() override {
    std::shared_ptr<MessageBase> message = std::make_shared<MessageType>(*dynamic_cast<MessageType*>(this));
    IMessageQueue::getInstance()->pushMessage(message);
  }

  virtual void dispatchImmediately() {
    std::shared_ptr<MessageBase> message = std::make_shared<MessageType>(*dynamic_cast<MessageType*>(this));
    IMessageQueue::getInstance()->processMessage(message, true);
  }

  void print(std::wostream& /*os*/) const override {}
};