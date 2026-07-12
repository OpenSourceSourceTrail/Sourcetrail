#pragma once

#include <memory>
#include <mutex>

#include "DispatchQueue.h"
#include "GlobalId.hpp"
#include "TaskRunner.h"

// Runs Task trees dispatched to one id, FIFO, on a single worker thread owned
// by an underlying DispatchQueue. Replaces TaskScheduler. A task tree is driven
// to a terminal state before the next queued task starts, so async task groups
// never overlap; only a task that returns STATE_HOLD (a background/delayed task)
// yields, re-posting itself to the back of the queue so others can interleave.
class TaskDispatchQueue final {
public:
  explicit TaskDispatchQueue(Id schedulerId);

  void pushTask(std::shared_ptr<Task> task);
  void pushNextTask(std::shared_ptr<Task> task);

  // Terminates whatever task is currently running and discards everything
  // still queued behind it.
  void terminateRunningTasks();

private:
  void runContinuation(const std::shared_ptr<TaskRunner>& runner);

  const Id mSchedulerId;
  DispatchQueue mQueue;

  std::mutex mCurrentRunnerMutex;
  std::shared_ptr<TaskRunner> mCurrentRunner;
};
