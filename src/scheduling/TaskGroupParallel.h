#pragma once
// STL
#include <atomic>
#include <map>
#include <thread>
// internal
#include "TaskGroup.h"
#include "TaskRunner.h"

class TaskGroupParallel : public TaskGroup {
public:
  TaskGroupParallel();

  void addTask(std::shared_ptr<Task> task) override;

private:
  struct TaskInfo {
    TaskInfo(std::shared_ptr<TaskRunner> taskRunner_) : taskRunner(taskRunner_), active(false) {}
    std::shared_ptr<TaskRunner> taskRunner;
    std::shared_ptr<std::thread> thread;
    volatile bool active;
  };

  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void doTerminate() override;

  void processTaskThreaded(std::shared_ptr<TaskInfo> taskInfo, std::shared_ptr<Blackboard> blackboard);
  int getActiveTaskCount() const;

  std::vector<std::shared_ptr<TaskInfo>> m_tasks;
  bool m_needsToStartThreads;

  volatile bool m_taskFailed;
  std::atomic<int> m_activeTaskCount;
};