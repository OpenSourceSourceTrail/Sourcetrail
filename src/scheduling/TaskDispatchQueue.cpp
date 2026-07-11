#include "TaskDispatchQueue.h"

TaskDispatchQueue::TaskDispatchQueue(Id schedulerId) : mSchedulerId(schedulerId) {}

void TaskDispatchQueue::pushTask(std::shared_ptr<Task> task) {
  auto runner = std::make_shared<TaskRunner>(std::move(task));
  mQueue.post([this, runner]() { runContinuation(runner); });
}

void TaskDispatchQueue::pushNextTask(std::shared_ptr<Task> task) {
  auto runner = std::make_shared<TaskRunner>(std::move(task));
  mQueue.postFront([this, runner]() { runContinuation(runner); });
}

void TaskDispatchQueue::terminateRunningTasks() {
  {
    std::lock_guard<std::mutex> lock(mCurrentRunnerMutex);
    if(mCurrentRunner) {
      mCurrentRunner->terminate();
    }
  }
  mQueue.clear();
}

void TaskDispatchQueue::runContinuation(const std::shared_ptr<TaskRunner>& runner) {
  {
    std::lock_guard<std::mutex> lock(mCurrentRunnerMutex);
    mCurrentRunner = runner;
  }

  const Task::TaskState state = runner->update(mSchedulerId);

  {
    std::lock_guard<std::mutex> lock(mCurrentRunnerMutex);
    mCurrentRunner = nullptr;
  }

  if(state == Task::STATE_HOLD) {
    mQueue.post([this, runner]() { runContinuation(runner); });
  }
}
