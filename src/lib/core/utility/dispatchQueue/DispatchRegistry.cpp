#include "DispatchRegistry.h"

#include <utility>

DispatchRegistry& DispatchRegistry::getInstance() noexcept {
  static DispatchRegistry sInstance;
  return sInstance;
}

DispatchQueue& DispatchRegistry::getQueue(Id id) noexcept {
  std::lock_guard<std::mutex> lock(mQueuesMutex);

  auto found = mQueues.find(id);
  if(found == mQueues.end()) {
    found = mQueues.emplace(id, std::make_unique<DispatchQueue>()).first;
  }

  return *found->second;
}

void DispatchRegistry::destroyQueue(Id id) noexcept {
  std::lock_guard<std::mutex> lock(mQueuesMutex);
  mQueues.erase(id);
}

void dispatch(Id schedulerId, std::function<void()> callback) noexcept {
  DispatchRegistry::getInstance().getQueue(schedulerId).post(std::move(callback));
}

void dispatchNext(Id schedulerId, std::function<void()> callback) noexcept {
  DispatchRegistry::getInstance().getQueue(schedulerId).postFront(std::move(callback));
}

void dispatchDelayed(Id schedulerId, std::function<void()> callback, std::chrono::milliseconds delayMs) noexcept {
  DispatchRegistry::getInstance().getQueue(schedulerId).postDelayed(std::move(callback), delayMs);
}
