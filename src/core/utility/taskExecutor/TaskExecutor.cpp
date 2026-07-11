#include "TaskExecutor.h"

#include <algorithm>
#include <thread>

tf::Executor& TaskExecutor::get() {
  // A plain function-local static gets its alignment honored directly by the
  // linker/runtime. tf::Executor contains cache-line-aligned internals
  // (tf::TaskQueue); embedding it inline in a make_shared<...>() allocation
  // caused a heap-buffer-overflow because the combined control-block
  // allocation didn't respect that over-alignment. Sharing one process-wide
  // executor also avoids spinning up a fresh N-thread pool per job.
  //
  // Some callers (e.g. the indexing pipeline's merge/inject branches) run a
  // blocking loop that never yields its worker until its whole subtree
  // finishes, and only make progress once another branch sets a flag. That
  // means every concurrently active branch needs its own worker at all
  // times, or the flag-setting branch can be starved forever (e.g. under
  // WSL2 / containers where hardware_concurrency() is small). Floor the pool
  // well above the handful of branches any single job ever adds.
  static tf::Executor sExecutor(std::max<unsigned>(std::thread::hardware_concurrency(), 8));
  return sExecutor;
}
