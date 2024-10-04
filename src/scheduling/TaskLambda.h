#pragma once
// STL
#include <functional>
// internal
#include "Task.h"

class TaskLambda : public Task {
public:
  TaskLambda(std::function<void()> func);

private:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;

  std::function<void()> m_func;
};