#include "TaskflowGroupParallel.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>

TaskflowGroupParallel::TaskflowGroupParallel() : m_needsToStartThreads(true), m_taskFailed(false) {}

tf::Executor& TaskflowGroupParallel::executor() {
  // A plain function-local static gets its alignment honored directly by the
  // linker/runtime. tf::Executor contains cache-line-aligned internals
  // (tf::TaskQueue); embedding it inline in a make_shared<TaskflowGroupParallel>()
  // allocation caused a heap-buffer-overflow because the combined control-block
  // allocation didn't respect that over-alignment. Sharing one process-wide
  // executor also avoids spinning up a fresh N-thread pool per indexing run.
  //
  // Each branch task runs a blocking while-loop that never yields its worker
  // until its whole subtree finishes, and some branches (merge/inject) only
  // make progress once another branch (TaskBuildIndex) sets a blackboard
  // flag. That means every concurrently active branch needs its own worker
  // at all times, or the flag-setting branch can be starved forever (e.g.
  // under WSL2 / containers where hardware_concurrency() is small). Floor
  // the pool well above the handful of branches indexing ever adds.
  static tf::Executor sExecutor(std::max<unsigned>(std::thread::hardware_concurrency(), 8));
  return sExecutor;
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
