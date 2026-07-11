#pragma once
#include <gmock/gmock.h>

#include "MessageQueue.h"

class MockedMessageQueue : public IMessageQueue {
public:
  MockedMessageQueue() noexcept = default;
  ~MockedMessageQueue() noexcept override = default;

  MOCK_METHOD(void, registerListener, (MessageListenerBase*), (noexcept, override));
  MOCK_METHOD(void, unregisterListener, (MessageListenerBase*), (noexcept, override));

  MOCK_METHOD(MessageListenerBase*, getListenerById, (Id), (const, noexcept, override));

  MOCK_METHOD(void, addMessageFilter, (std::shared_ptr<MessageFilter>), (noexcept, override));

  MOCK_METHOD(void, pushMessage, (std::shared_ptr<MessageBase>), (noexcept, override));
  MOCK_METHOD(void, processMessage, (const std::shared_ptr<MessageBase>&, bool), (noexcept, override));

  MOCK_METHOD(void, startMessageLoopThreaded, (), (noexcept, override));
  MOCK_METHOD(void, startMessageLoop, (), (noexcept, override));
  MOCK_METHOD(void, stopMessageLoop, (), (noexcept, override));

  MOCK_METHOD(bool, loopIsRunning, (), (const, noexcept, override));
  MOCK_METHOD(bool, hasMessagesQueued, (), (const, noexcept, override));

  MOCK_METHOD(void, setSendMessagesAsTasks, (bool), (noexcept, override));
};