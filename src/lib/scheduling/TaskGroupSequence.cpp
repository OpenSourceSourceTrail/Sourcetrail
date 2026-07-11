#include "TaskGroupSequence.h"

#include <range/v3/algorithm/for_each.hpp>

TaskGroupSequence::TaskGroupSequence() = default;

void TaskGroupSequence::addTask(std::shared_ptr<Task> task) {
  m_taskRunners.push_back(std::make_shared<TaskRunner>(task));
}

void TaskGroupSequence::doEnter(std::shared_ptr<Blackboard> /*blackboard*/) {
  m_taskIndex = 0;
}

Task::TaskState TaskGroupSequence::doUpdate(std::shared_ptr<Blackboard> blackboard) {
  if(m_taskIndex >= int(m_taskRunners.size())) {
    return STATE_SUCCESS;
  } else if(m_taskIndex < 0) {
    return STATE_FAILURE;
  }

  TaskState state = m_taskRunners[static_cast<size_t>(m_taskIndex)]->update(blackboard);

  if(state == STATE_SUCCESS) {
    m_taskIndex++;
  } else if(state == STATE_FAILURE) {
    m_taskIndex = -1;
  } else if(state == STATE_HOLD) {
    return STATE_HOLD;
  }

  return STATE_RUNNING;
}

void TaskGroupSequence::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskGroupSequence::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {
  ranges::for_each(m_taskRunners, [](auto& taskRunner) { taskRunner->reset(); });
}

void TaskGroupSequence::doTerminate() {
  ranges::for_each(m_taskRunners, [](auto& taskRunner) { taskRunner->terminate(); });
}