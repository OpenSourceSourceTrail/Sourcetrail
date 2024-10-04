#include "TaskManager.hpp"

#include "TaskScheduler.h"

namespace scheduling::impls {

std::shared_ptr<TaskScheduler> TaskManager::createScheduler(GlobalId schedulerId) noexcept {
  return getScheduler(schedulerId);
}

void TaskManager::destroyScheduler(GlobalId schedulerId) noexcept {
  std::lock_guard<std::mutex> lock(mSchedulersMutex);

  if(auto found = mSchedulers.find(schedulerId); found != mSchedulers.end()) {
    mSchedulers.erase(found);
  }
}

std::shared_ptr<TaskScheduler> TaskManager::getScheduler(GlobalId schedulerId) noexcept {
  std::lock_guard<std::mutex> lock(mSchedulersMutex);

  if(auto found = mSchedulers.find(schedulerId); found != mSchedulers.end()) {
    return found->second;
  }

  auto scheduler = std::make_shared<TaskScheduler>(schedulerId);
  mSchedulers.emplace(schedulerId, scheduler);
  return scheduler;
}

}    // namespace lib::detail