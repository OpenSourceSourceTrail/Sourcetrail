#pragma once
// internal
#include "MessageQueue.h"

class MessageFilter {
public:
  virtual ~MessageFilter() = default;

  virtual void filter(IMessageQueue::MessageBufferType* pMessageBuffer) = 0;
};