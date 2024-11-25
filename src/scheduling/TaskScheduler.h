#pragma once

#include <deque>
#include <memory>
#include <mutex>

#include "Task.h"
#include "TaskRunner.h"
#include "GlobalId.hpp"

class TaskScheduler final {
public:
  explicit TaskScheduler(Id schedulerId);
  ~TaskScheduler();

  void pushTask(std::shared_ptr<Task> task);
  void pushNextTask(std::shared_ptr<Task> task);

  void startSchedulerLoopThreaded();
  void startSchedulerLoop();
  void stopSchedulerLoop();

  bool loopIsRunning() const;
  bool hasTasksQueued() const;

  void terminateRunningTasks();

private:
  void processTasks();

  const Id m_schedulerId;

  bool m_loopIsRunning;
  bool m_threadIsRunning;
  bool m_terminateRunningTasks;

  std::deque<std::shared_ptr<TaskRunner>> m_taskRunners;

  mutable std::mutex m_tasksMutex;
  mutable std::mutex m_loopMutex;
  mutable std::mutex m_threadMutex;
};