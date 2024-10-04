#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "../Blackboard.h"
#include "../Task.h"
#include "../TaskGroupSelector.h"
#include "../TaskGroupSequence.h"
#include "../TaskScheduler.h"

namespace {
void executeTask(Task& task) {
  std::shared_ptr<Blackboard> blackboard = std::make_shared<Blackboard>();
  while(true) {
    if(task.update(blackboard) != Task::STATE_RUNNING) {
      return;
    }
  }
}

class TestTask : public Task {
public:
  TestTask(int* orderCountPtr_, int updateCount_, TaskState returnState_ = STATE_SUCCESS)
      : orderCount(*orderCountPtr_), updateCount(updateCount_), returnState(returnState_) {}

  void doEnter(std::shared_ptr<Blackboard> /*blackboard*/) override {
    enterCallOrder = ++orderCount;
  }

  TaskState doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) override {
    updateCallOrder = ++orderCount;

    if(updateCount < 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return Task::STATE_RUNNING;
    }

    updateCount--;
    if(updateCount != 0) {
      return Task::STATE_RUNNING;
    }

    return returnState;
  }

  void doExit(std::shared_ptr<Blackboard> /*blackboard*/) override {
    exitCallOrder = ++orderCount;
  }

  void doReset(std::shared_ptr<Blackboard> /*blackboard*/) override {
    resetCallOrder = ++orderCount;
  }

  int& orderCount;
  int updateCount;
  TaskState returnState;

  int enterCallOrder {};
  int updateCallOrder {};
  int exitCallOrder {};
  int resetCallOrder {};
};

class TestTaskDispatch : public TestTask {
public:
  TestTaskDispatch(int* orderCountPtr, int updateCount_, TaskScheduler* scheduler)
      : TestTask(orderCountPtr, updateCount_), mScheduler(scheduler) {}

  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override {
    subTask = std::make_shared<TestTask>(&orderCount, 1);
    mScheduler->pushTask(subTask);

    return TestTask::doUpdate(blackboard);
  }

  TaskScheduler* mScheduler;
  std::shared_ptr<TestTask> subTask;
};

void waitForThread(TaskScheduler& scheduler) {
  static const int THREAD_WAIT_TIME_MS = 20;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT_TIME_MS));
  } while(scheduler.hasTasksQueued());
}
}    // namespace

TEST(TaskScheduler, loopStartsAndStops) {
  TaskScheduler scheduler(0);
  EXPECT_TRUE(!scheduler.loopIsRunning());

  scheduler.startSchedulerLoopThreaded();

  waitForThread(scheduler);

  EXPECT_TRUE(scheduler.loopIsRunning());

  scheduler.stopSchedulerLoop();

  waitForThread(scheduler);

  EXPECT_TRUE(!scheduler.loopIsRunning());
}

TEST(TaskScheduler, tasksGetExecutedWithoutSchedulingInCorrectOrder) {
  int order = 0;
  TestTask task(&order, 1);

  executeTask(task);

  EXPECT_TRUE(3 == order);

  EXPECT_TRUE(1 == task.enterCallOrder);
  EXPECT_TRUE(2 == task.updateCallOrder);
  EXPECT_TRUE(3 == task.exitCallOrder);
}

TEST(TaskScheduler, tasksGetProcessedWithCallbacksInCorrectOrder) {
  TaskScheduler scheduler(0);
  scheduler.startSchedulerLoopThreaded();

  int order = 0;
  std::shared_ptr<TestTask> task = std::make_shared<TestTask>(&order, 1);

  scheduler.pushTask(task);

  waitForThread(scheduler);

  scheduler.stopSchedulerLoop();

  EXPECT_TRUE(3 == order);

  EXPECT_TRUE(1 == task->enterCallOrder);
  EXPECT_TRUE(2 == task->updateCallOrder);
  EXPECT_TRUE(3 == task->exitCallOrder);
}

TEST(TaskScheduler, sequentialTaskGroupToProcessTasksInCorrectOrder) {
  TaskScheduler scheduler(0);
  scheduler.startSchedulerLoopThreaded();

  int order = 0;
  std::shared_ptr<TestTask> task1 = std::make_shared<TestTask>(&order, 1);
  std::shared_ptr<TestTask> task2 = std::make_shared<TestTask>(&order, 1);

  std::shared_ptr<TaskGroupSequence> taskGroup = std::make_shared<TaskGroupSequence>();
  taskGroup->addTask(task1);
  taskGroup->addTask(task2);

  scheduler.pushTask(taskGroup);

  waitForThread(scheduler);

  scheduler.stopSchedulerLoop();

  EXPECT_TRUE(6 == order);

  EXPECT_TRUE(1 == task1->enterCallOrder);
  EXPECT_TRUE(2 == task1->updateCallOrder);
  EXPECT_TRUE(3 == task1->exitCallOrder);

  EXPECT_TRUE(4 == task2->enterCallOrder);
  EXPECT_TRUE(5 == task2->updateCallOrder);
  EXPECT_TRUE(6 == task2->exitCallOrder);
}

TEST(TaskScheduler, sequentialTaskGroupDoesNotEvaluateTasksAfterFailure) {
  TaskScheduler scheduler(0);
  scheduler.startSchedulerLoopThreaded();

  int order = 0;
  std::shared_ptr<TestTask> task1 = std::make_shared<TestTask>(&order, 1, Task::STATE_FAILURE);
  std::shared_ptr<TestTask> task2 = std::make_shared<TestTask>(&order, -1);

  std::shared_ptr<TaskGroupSequence> taskGroup = std::make_shared<TaskGroupSequence>();
  taskGroup->addTask(task1);
  taskGroup->addTask(task2);

  scheduler.pushTask(taskGroup);

  waitForThread(scheduler);

  scheduler.stopSchedulerLoop();

  EXPECT_TRUE(1 == task1->enterCallOrder);
  EXPECT_TRUE(2 == task1->updateCallOrder);
  EXPECT_TRUE(3 == task1->exitCallOrder);

  EXPECT_TRUE(0 == task2->enterCallOrder);
  EXPECT_TRUE(0 == task2->updateCallOrder);
  EXPECT_TRUE(0 == task2->exitCallOrder);
}

TEST(TaskScheduler, sequentialTaskGroupDoesNotEvaluateTasksAfterSuccess) {
  TaskScheduler scheduler(0);
  scheduler.startSchedulerLoopThreaded();

  int order = 0;
  std::shared_ptr<TestTask> task1 = std::make_shared<TestTask>(&order, 1, Task::STATE_FAILURE);
  std::shared_ptr<TestTask> task2 = std::make_shared<TestTask>(&order, 1, Task::STATE_SUCCESS);
  std::shared_ptr<TestTask> task3 = std::make_shared<TestTask>(&order, -1);

  std::shared_ptr<TaskGroupSelector> taskGroup = std::make_shared<TaskGroupSelector>();
  taskGroup->addTask(task1);
  taskGroup->addTask(task2);
  taskGroup->addTask(task3);

  scheduler.pushTask(taskGroup);

  waitForThread(scheduler);

  scheduler.stopSchedulerLoop();

  EXPECT_TRUE(1 == task1->enterCallOrder);
  EXPECT_TRUE(2 == task1->updateCallOrder);
  EXPECT_TRUE(3 == task1->exitCallOrder);

  EXPECT_TRUE(4 == task2->enterCallOrder);
  EXPECT_TRUE(5 == task2->updateCallOrder);
  EXPECT_TRUE(6 == task2->exitCallOrder);

  EXPECT_TRUE(0 == task3->enterCallOrder);
  EXPECT_TRUE(0 == task3->updateCallOrder);
  EXPECT_TRUE(0 == task3->exitCallOrder);
}

TEST(TaskScheduler, taskSchedulingWithinTaskProcessing) {
  TaskScheduler scheduler(0);
  scheduler.startSchedulerLoopThreaded();

  int order = 0;
  std::shared_ptr<TestTaskDispatch> task = std::make_shared<TestTaskDispatch>(&order, 1, &scheduler);

  scheduler.pushTask(task);

  waitForThread(scheduler);

  scheduler.stopSchedulerLoop();

  EXPECT_TRUE(6 == order);

  EXPECT_TRUE(1 == task->enterCallOrder);
  EXPECT_TRUE(2 == task->updateCallOrder);
  EXPECT_TRUE(3 == task->exitCallOrder);

  EXPECT_TRUE(4 == task->subTask->enterCallOrder);
  EXPECT_TRUE(5 == task->subTask->updateCallOrder);
  EXPECT_TRUE(6 == task->subTask->exitCallOrder);
}