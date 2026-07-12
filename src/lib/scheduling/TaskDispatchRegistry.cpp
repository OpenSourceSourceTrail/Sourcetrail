#include "TaskDispatchRegistry.h"

TaskDispatchRegistry& TaskDispatchRegistry::getInstance() noexcept {
  static TaskDispatchRegistry sInstance;
  return sInstance;
}

TaskDispatchQueue& TaskDispatchRegistry::getQueue(Id schedulerId) noexcept {
  std::lock_guard<std::mutex> lock(mQueuesMutex);

  auto found = mQueues.find(schedulerId);
  if(found == mQueues.end()) {
    found = mQueues.emplace(schedulerId, std::make_unique<TaskDispatchQueue>(schedulerId)).first;
  }

  return *found->second;
}

void TaskDispatchRegistry::destroyQueue(Id schedulerId) noexcept {
  std::lock_guard<std::mutex> lock(mQueuesMutex);
  mQueues.erase(schedulerId);
}
