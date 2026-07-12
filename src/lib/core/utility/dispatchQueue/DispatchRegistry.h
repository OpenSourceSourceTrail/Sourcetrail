#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include "DispatchQueue.h"
#include "GlobalId.hpp"

// Owns one DispatchQueue per id. Replaces ITaskManager/impls::TaskManager/TaskScheduler.
class DispatchRegistry final {
public:
  static DispatchRegistry& getInstance() noexcept;

  DispatchRegistry(const DispatchRegistry&) = delete;
  DispatchRegistry& operator=(const DispatchRegistry&) = delete;

  // Creates the queue for id if it doesn't exist yet, and returns it either way.
  DispatchQueue& getQueue(Id id) noexcept;

  void destroyQueue(Id id) noexcept;

private:
  DispatchRegistry() = default;

  std::map<Id, std::unique_ptr<DispatchQueue>> mQueues;
  std::mutex mQueuesMutex;
};

// Convenience free functions mirroring the old Task::dispatch / Task::dispatchNext.
void dispatch(Id schedulerId, std::function<void()> callback) noexcept;
void dispatchNext(Id schedulerId, std::function<void()> callback) noexcept;
void dispatchDelayed(Id schedulerId, std::function<void()> callback, std::chrono::milliseconds delayMs) noexcept;
