#pragma once
#include <mutex>

#include <condition_variable>

#include "MessageListener.h"
#include "status/messages/MessageStatus.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "type/MessageQuitApplication.h"

// Qt-free replacement for QtCoreApplication: keeps the process alive until
// a MessageQuitApplication is dispatched.
class ConsoleApplication final
    : public MessageListener<MessageQuitApplication>
    , public MessageListener<MessageIndexingStatus>
    , public MessageListener<MessageStatus> {
public:
  ConsoleApplication() = default;

  void run();

private:
  void handleMessage(MessageQuitApplication* message) override;
  void handleMessage(MessageIndexingStatus* message) override;
  void handleMessage(MessageStatus* message) override;

  std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_quit = false;
};
