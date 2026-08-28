#include <gtest/gtest.h>

#include "data/indexer/TaskBuildIndex.h"

using namespace std::chrono_literals;

TEST(TaskBuildIndex, crashLoopExitIsAnImmediateFailureOnly) {
  // The worker died before it could pull any work: respawning it at once is a busy loop.
  EXPECT_TRUE(TaskBuildIndex::isCrashLoopExit(1, 10ms));

  // A clean exit is how a worker reports "queue drained", whether it was quick or not.
  EXPECT_FALSE(TaskBuildIndex::isCrashLoopExit(0, 10ms));
  EXPECT_FALSE(TaskBuildIndex::isCrashLoopExit(0, 60s));

  // A worker that ran a while indexed something first, so the failure is not a loop -- retry it.
  EXPECT_FALSE(TaskBuildIndex::isCrashLoopExit(1, 60s));
}
