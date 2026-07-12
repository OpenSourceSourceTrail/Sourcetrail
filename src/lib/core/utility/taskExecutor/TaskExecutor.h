#pragma once

#include <taskflow/taskflow.hpp>

// Process-wide tf::Executor. One instance is shared by every DAG in the
// process (indexing pipeline, MessageQueue parallel broadcast, ...) instead
// of spinning up a fresh thread pool per job.
class TaskExecutor final {
public:
  static tf::Executor& get();
};
