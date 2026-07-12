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

  // STATE_RUNNING means the task (e.g. a TaskGroupSequence/TaskflowGroupParallel) has more work
  // to do and must be driven forward. Drive it to completion here before returning to the queue:
  // an async group (TaskflowGroupParallel) leaves its worker threads running while it reports
  // STATE_RUNNING, so starting the next queued task now would run a second task tree concurrently
  // and race on shared controller state (e.g. GraphController's dummy-node map). This mirrors the
  // old TaskScheduler, which looped on the task until it left STATE_RUNNING.
  Task::TaskState state = runner->update(mSchedulerId);
  while(state == Task::STATE_RUNNING) {
    state = runner->update(mSchedulerId);
  }

  {
    std::lock_guard<std::mutex> lock(mCurrentRunnerMutex);
    mCurrentRunner = nullptr;
  }

  // STATE_HOLD is a cooperative yield (background/delayed tasks); re-post to the back so other
  // queued tasks can interleave, then resume this one later. SUCCESS/FAILURE are terminal.
  if(state == Task::STATE_HOLD) {
    mQueue.post([this, runner]() { runContinuation(runner); });
  }
}
