#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "GlobalId.hpp"
#include "TaskDispatchQueue.h"

// Owns one TaskDispatchQueue per scheduler id. Replaces
// ITaskManager/impls::TaskManager.
class TaskDispatchRegistry final {
public:
  static TaskDispatchRegistry& getInstance() noexcept;

  TaskDispatchRegistry(const TaskDispatchRegistry&) = delete;
  TaskDispatchRegistry& operator=(const TaskDispatchRegistry&) = delete;

  // Creates the queue for id if it doesn't exist yet, and returns it either way.
  TaskDispatchQueue& getQueue(Id schedulerId) noexcept;

  void destroyQueue(Id schedulerId) noexcept;

private:
  TaskDispatchRegistry() = default;

  std::map<Id, std::unique_ptr<TaskDispatchQueue>> mQueues;
  std::mutex mQueuesMutex;
};
