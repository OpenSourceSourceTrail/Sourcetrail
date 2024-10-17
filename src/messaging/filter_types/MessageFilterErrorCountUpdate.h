#pragma once

#include "MessageFilter.h"
#include "type/error/MessageErrorCountUpdate.h"

class MessageFilterErrorCountUpdate : public MessageFilter {
  void filter(IMessageQueue::MessageBufferType* messageBuffer) override {
    if(messageBuffer->size() < 2) {
      return;
    }

    MessageBase* message = messageBuffer->front().get();
    if(message->getType() == MessageErrorCountUpdate::getStaticType()) {
      for(auto it = messageBuffer->begin() + 1; it != messageBuffer->end(); it++) {
        if((*it)->getType() == MessageErrorCountUpdate::getStaticType()) {
          MessageErrorCountUpdate* frontErrorsMessage = dynamic_cast<MessageErrorCountUpdate*>(message);
          MessageErrorCountUpdate* backErrorsMessage = dynamic_cast<MessageErrorCountUpdate*>(it->get());

          backErrorsMessage->newErrors.insert(
              backErrorsMessage->newErrors.begin(), frontErrorsMessage->newErrors.begin(), frontErrorsMessage->newErrors.end());

          messageBuffer->pop_front();
          return;
        }
      }
    }
  }
};