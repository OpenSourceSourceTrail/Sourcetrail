#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "impls/TaskManager.hpp"

using testing::IsNull;
using testing::Not;
using testing::Test;

namespace scheduling::details {

TEST(TaskManagerTestSuite, createScheduler) {
  // Given: TaskManger is created
  scheduling::impls::TaskManager taskManager;

  // When: create a task with id = 100
  const GlobalId id {100};
  auto task = taskManager.createScheduler(id);
  ASSERT_THAT(task, Not(IsNull()));

  // And: create a task with same id
  auto sameTask = taskManager.createScheduler(id);
  ASSERT_THAT(sameTask, Not(IsNull()));

  // Then: One task must be created
  EXPECT_EQ(task.get(), sameTask.get());
}

TEST(TaskManagerTestSuite, destroySchedulerNonExistsing) {
  // Given: TaskManger is created
  scheduling::impls::TaskManager taskManager;

  // When: destroy a non-created task
  const GlobalId id {100};
  EXPECT_NO_FATAL_FAILURE(taskManager.destroyScheduler(id));

  // Then: One crash should happen
}

TEST(TaskManagerTestSuite, destroyScheduler) {
  // Given: TaskManger is created
  scheduling::impls::TaskManager taskManager;

  // And: A task with 100 is created
  const GlobalId id {100};
  auto task = taskManager.createScheduler(id);
  ASSERT_THAT(task, Not(IsNull()));
  ASSERT_EQ(2, task.use_count());

  // When: Destroying the task with id = 10
  taskManager.destroyScheduler(id);

  // Then: the created task should be removed from the map
  EXPECT_EQ(1, task.use_count());
}

}    // namespace scheduling::details