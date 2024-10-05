#pragma once
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>
#include <cassert>

#include "types.h"

class MessageBase;
class MessageFilter;
class MessageListenerBase;

struct IMessageQueue {
  using Ptr = std::shared_ptr<IMessageQueue>;
  using MessageBufferType = std::deque<std::shared_ptr<MessageBase>>;

  static Ptr getInstance() {
    assert(sInstance);
    return sInstance;
  }

  static IMessageQueue* getInstanceRaw() {
    assert(sInstance);
    return sInstance.get();
  }

  static void setInstance(Ptr instance) {
    sInstance = std::move(instance);
  }

  IMessageQueue() noexcept = default;

  IMessageQueue(const IMessageQueue&) = delete;
  IMessageQueue(IMessageQueue&&) = delete;
  IMessageQueue& operator=(const IMessageQueue&) = delete;
  IMessageQueue& operator=(IMessageQueue&&) = delete;

  virtual ~IMessageQueue() noexcept = default;

  virtual void registerListener(MessageListenerBase* listener) noexcept = 0;
  virtual void unregisterListener(MessageListenerBase* listener) noexcept = 0;

  [[nodiscard]] virtual MessageListenerBase* getListenerById(Id listenerId) const noexcept = 0;

  virtual void addMessageFilter(std::shared_ptr<MessageFilter> filter) noexcept = 0;

  virtual void pushMessage(std::shared_ptr<MessageBase> message) noexcept = 0;
  virtual void processMessage(const std::shared_ptr<MessageBase>&, bool asNextTask) noexcept = 0;

  virtual void startMessageLoopThreaded() noexcept = 0;
  virtual void startMessageLoop() noexcept = 0;
  virtual void stopMessageLoop() noexcept = 0;

  [[nodiscard]] virtual bool loopIsRunning() const noexcept = 0;
  [[nodiscard]] virtual bool hasMessagesQueued() const noexcept = 0;

  virtual void setSendMessagesAsTasks(bool sendMessagesAsTasks) noexcept = 0;

private:
  static Ptr sInstance;
};

namespace details {
class MessageQueue final : public IMessageQueue {
public:
  MessageQueue() noexcept;

  MessageQueue(const MessageQueue&) = delete;
  MessageQueue(MessageQueue&&) = delete;
  MessageQueue& operator=(const MessageQueue&) = delete;
  MessageQueue& operator=(MessageQueue&&) = delete;

  ~MessageQueue() noexcept override;

  void registerListener(MessageListenerBase* listener) noexcept override;
  void unregisterListener(MessageListenerBase* listener) noexcept override;

  [[nodiscard]] MessageListenerBase* getListenerById(Id listenerId) const noexcept override;

  void addMessageFilter(std::shared_ptr<MessageFilter> filter) noexcept override;

  void pushMessage(std::shared_ptr<MessageBase> message) noexcept override;
  void processMessage(const std::shared_ptr<MessageBase>&, bool asNextTask) noexcept override;

  void startMessageLoopThreaded() noexcept override;
  void startMessageLoop() noexcept override;
  void stopMessageLoop() noexcept override;

  bool loopIsRunning() const noexcept override;
  bool hasMessagesQueued() const noexcept override;

  void setSendMessagesAsTasks(bool sendMessagesAsTasks) noexcept override;

private:
  void processMessages();
  void sendMessage(const std::shared_ptr<MessageBase>& message);
  void sendMessageAsTask(const std::shared_ptr<MessageBase>& message, bool asNextTask) const;

  MessageBufferType mMessageBuffer;
  std::vector<MessageListenerBase*> mListeners;
  std::vector<std::shared_ptr<MessageFilter>> mFilters;

  size_t mCurrentListenerIndex = 0;
  size_t mListenersLength = 0;

  std::atomic_bool mLoopIsRunning = false;
  std::atomic_bool mThreadIsRunning = false;

  mutable std::mutex mMessageBufferMutex;
  mutable std::mutex mListenersMutex;

  bool mSendMessagesAsTasks = false;
};
}    // namespace details