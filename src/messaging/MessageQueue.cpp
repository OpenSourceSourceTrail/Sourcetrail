#include "MessageQueue.h"

#include <chrono>
#include <mutex>
#include <thread>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>

#include "../../../scheduling/TaskGroupParallel.h"
#include "../../../scheduling/TaskGroupSequence.h"
#include "../../../scheduling/TaskLambda.h"
#include "logging.h"
#include "MessageBase.h"
#include "MessageFilter.h"
#include "MessageListenerBase.h"
#include "TabId.h"

IMessageQueue::Ptr IMessageQueue::sInstance;

namespace details {
MessageQueue::MessageQueue() noexcept = default;

MessageQueue::~MessageQueue() noexcept {
  std::scoped_lock<std::mutex> lock(mListenersMutex);
  ranges::for_each(mListeners, [](auto& listener) { listener->removedListener(); });
  mListeners.clear();
}

void MessageQueue::registerListener(MessageListenerBase* listener) noexcept {
  std::scoped_lock<std::mutex> lock(mListenersMutex);
  if(ranges::find(mListeners, listener) != mListeners.end()) {
    return;
  }
  mListeners.push_back(listener);
}

void MessageQueue::unregisterListener(MessageListenerBase* listener) noexcept {
  std::scoped_lock<std::mutex> lock(mListenersMutex);
  if(auto found = ranges::find(mListeners, listener); found != mListeners.end()) {
    mListeners.erase(found);

    // TODO(Hussein): That sound fishy for me. It need `mListenersMutex`
    const auto index = static_cast<size_t>(std::distance(mListeners.begin(), found));
    // mCurrentListenerIndex and mListenersLength need to be updated in case this happens
    // while a message is handled.
    if(index <= mCurrentListenerIndex) {
      mCurrentListenerIndex--;
    }

    if(index < mListenersLength) {
      mListenersLength--;
    }
  }

  LOG_ERROR("Listener was not found");
}

MessageListenerBase* MessageQueue::getListenerById(Id listenerId) const noexcept {
  std::scoped_lock<std::mutex> lock(mListenersMutex);
  auto found = ranges::find_if(mListeners, [listenerId](auto* listener) { return listener->getId() == listenerId; });
  return found == mListeners.end() ? nullptr : *found;
}

void MessageQueue::addMessageFilter(std::shared_ptr<MessageFilter> filter) noexcept {
  if(ranges::find(mFilters, filter) != mFilters.end()) {
    return;
  }
  mFilters.push_back(std::move(filter));
}

void MessageQueue::pushMessage(std::shared_ptr<MessageBase> message) noexcept {
  std::scoped_lock<std::mutex> lock(mMessageBufferMutex);
  if(ranges::find(mMessageBuffer, message) != mMessageBuffer.end()) {
    return;
  }
  mMessageBuffer.push_back(std::move(message));
}

void MessageQueue::processMessage(const std::shared_ptr<MessageBase>& message, bool asNextTask) noexcept {
  if(message->isLogged()) {
    LOG_INFO_W(L"send " + message->str());
  }

  if(mSendMessagesAsTasks && message->sendAsTask()) {
    sendMessageAsTask(message, asNextTask);
    return;
  }

  sendMessage(message);
}

void MessageQueue::startMessageLoopThreaded() noexcept {
  // TODO(Hussein): Remove `detach()`
  std::thread(&MessageQueue::startMessageLoop, this).detach();
  mThreadIsRunning = true;
}

void MessageQueue::startMessageLoop() noexcept {
  if(mLoopIsRunning) {
    LOG_ERROR("Loop is already running");
    return;
  }
  mLoopIsRunning = true;

  while(true) {
    processMessages();

    if(!mLoopIsRunning) {
      break;
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(25ms);
  }

  if(mThreadIsRunning) {
    mThreadIsRunning = false;
  }
}

void MessageQueue::stopMessageLoop() noexcept {
  if(!mLoopIsRunning) {
    LOG_WARNING("Loop is not running");
  }

  mLoopIsRunning = false;

  while(true) {
    if(!mThreadIsRunning) {
      break;
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(25ms);
  }
}

bool MessageQueue::loopIsRunning() const noexcept {
  return mLoopIsRunning;
}

bool MessageQueue::hasMessagesQueued() const noexcept {
  std::scoped_lock<std::mutex> lock(mMessageBufferMutex);
  return !mMessageBuffer.empty();
}

void MessageQueue::setSendMessagesAsTasks(bool sendMessagesAsTasks) noexcept {
  mSendMessagesAsTasks = sendMessagesAsTasks;
}

void MessageQueue::processMessages() {
  while(true) {
    std::shared_ptr<MessageBase> message;
    {
      std::scoped_lock<std::mutex> lock(mMessageBufferMutex);

      ranges::for_each(mFilters, [this](const auto& filter) {
        if(mMessageBuffer.empty()) {
          return;
        }

        filter->filter(&mMessageBuffer);
      });

      if(mMessageBuffer.empty()) {
        break;
      }

      message = mMessageBuffer.front();
      mMessageBuffer.pop_front();
    }

    processMessage(message, false);
  }
}

void MessageQueue::sendMessage(const std::shared_ptr<MessageBase>& message) {
  std::scoped_lock<std::mutex> lock(mListenersMutex);

  // mListenersLength is saved, so that new listeners registered within message handling don't
  // get the current message and the length can be reduced when a listener gets unregistered.
  mListenersLength = mListeners.size();

  // The currentListenerIndex holds the index of the current listener being handled, so it can be
  // changed when a listener gets removed while message handling.
  for(mCurrentListenerIndex = 0; mCurrentListenerIndex < mListenersLength; ++mCurrentListenerIndex) {
    MessageListenerBase* listener = mListeners[mCurrentListenerIndex];

    if(listener->getType() == message->getType() &&
       (message->getSchedulerId() == 0 || listener->getSchedulerId() == 0 ||
        listener->getSchedulerId() == message->getSchedulerId())) {
      // The listenersMutex gets unlocked so changes to listeners are possible while message handling.
      mListenersMutex.unlock();
      listener->handleMessageBase(message.get());
      mListenersMutex.lock();
    }
  }
}

void MessageQueue::sendMessageAsTask(const std::shared_ptr<MessageBase>& message, bool asNextTask) const {
  std::shared_ptr<TaskGroup> taskGroup;
  if(message->isParallel()) {
    taskGroup = std::make_shared<TaskGroupParallel>();
  } else {
    taskGroup = std::make_shared<TaskGroupSequence>();
  }

  {
    std::scoped_lock<std::mutex> lock(mListenersMutex);
    for(auto* pListener : mListeners) {
      if(pListener->getType() == message->getType() &&
         (message->getSchedulerId() == 0 || pListener->getSchedulerId() == 0 ||
          pListener->getSchedulerId() == message->getSchedulerId())) {
        Id listenerId = pListener->getId();
        taskGroup->addTask(std::make_shared<TaskLambda>([listenerId, message]() {
          auto* pInnerListener = MessageQueue::getInstance()->getListenerById(listenerId);
          if(pInnerListener != nullptr) {
            pInnerListener->handleMessageBase(message.get());
          }
        }));
      }
    }
  }

  Id schedulerId = message->getSchedulerId();
  if(schedulerId == 0U) {
    schedulerId = TabId::app();
  }

  if(asNextTask) {
    Task::dispatchNext(schedulerId, taskGroup);
  } else {
    Task::dispatch(schedulerId, taskGroup);
  }
}
}    // namespace details