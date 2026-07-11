#pragma once
// STL
#include <atomic>
#include <vector>
// external
#include <taskflow/taskflow.hpp>
// internal
#include "TaskGroup.h"
#include "TaskRunner.h"

// Like TaskGroupParallel, but drives each branch through a taskflow::Executor
// work-stealing pool instead of one std::thread per branch.
class TaskflowGroupParallel : public TaskGroup {
public:
  TaskflowGroupParallel();

  void addTask(std::shared_ptr<Task> task) override;

private:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void doTerminate() override;

  static tf::Executor& executor();

  std::vector<std::shared_ptr<TaskRunner>> m_taskRunners;

  tf::Taskflow m_taskflow;
  tf::Future<void> m_future;

  bool m_needsToStartThreads;
  std::atomic<bool> m_taskFailed;
};
