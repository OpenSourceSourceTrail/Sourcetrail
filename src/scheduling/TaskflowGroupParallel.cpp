#include "TaskflowGroupParallel.h"

#include <chrono>
#include <future>

#include "TaskExecutor.h"

TaskflowGroupParallel::TaskflowGroupParallel() : m_needsToStartThreads(true), m_taskFailed(false) {}

tf::Executor& TaskflowGroupParallel::executor() {
  return TaskExecutor::get();
}

void TaskflowGroupParallel::addTask(std::shared_ptr<Task> task) {
  m_taskRunners.push_back(std::make_shared<TaskRunner>(task));
}

void TaskflowGroupParallel::doEnter(std::shared_ptr<Blackboard> blackboard) {
  m_taskFailed = false;

  if(m_needsToStartThreads) {
    m_needsToStartThreads = false;

    for(const std::shared_ptr<TaskRunner>& runner : m_taskRunners) {
      m_taskflow.emplace([this, runner, blackboard]() {
        while(true) {
          const Task::TaskState state = runner->update(blackboard);
          if(state == Task::STATE_SUCCESS || state == Task::STATE_FAILURE) {
            if(state == Task::STATE_FAILURE) {
              m_taskFailed = true;
            }
            break;
          }
        }
      });
    }

    m_future = executor().run(m_taskflow);
  }
}

Task::TaskState TaskflowGroupParallel::doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(!m_taskRunners.empty() && m_future.wait_for(std::chrono::milliseconds(25)) != std::future_status::ready) {
    return STATE_RUNNING;
  }

  return (m_taskFailed ? STATE_FAILURE : STATE_SUCCESS);
}

void TaskflowGroupParallel::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(m_future.valid()) {
    m_future.get();
  }
}

void TaskflowGroupParallel::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {
  m_taskFailed = false;

  for(const std::shared_ptr<TaskRunner>& runner : m_taskRunners) {
    runner->reset();
  }

  m_future = executor().run(m_taskflow);
}

void TaskflowGroupParallel::doTerminate() {
  for(const std::shared_ptr<TaskRunner>& runner : m_taskRunners) {
    runner->terminate();
  }

  if(m_future.valid()) {
    m_future.cancel();
    m_future.get();
  }
}
