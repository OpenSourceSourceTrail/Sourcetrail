#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

/**
 * Wall-clock split of the engine-side indexing pipeline, printed once per index run as an
 * INDEXER_TIMING line so scripts/bench_index.sh picks it up next to the worker lines.
 *
 * ponytail: process-global counters, never reset. One index run per process (the CLI, the engine
 * daemon between projects) is all this has to cover; per-run scoping would need plumbing that
 * buys nothing today.
 */
namespace indexing_stats {

/**
 * Backoff a merge/inject iteration takes when it found nothing to do. The repeat decorator used to
 * sleep this after *every* iteration, successful ones included, which capped merges at 4/s and
 * injections at 40/s no matter how fast they ran. The stages now sleep it themselves, on the
 * no-work path only.
 */
constexpr std::size_t MergeStorageIdleDelayMs = 20;
constexpr std::size_t InjectStorageIdleDelayMs = 5;

/** A stage: how many times it ran and how long it spent doing so. */
struct Phase {
  std::atomic<std::size_t> count{0};
  std::atomic<double> milliseconds{0.0};

  void add(double elapsedMs) noexcept {
    count.fetch_add(1, std::memory_order_relaxed);
    // fetch_add on a double needs a CAS loop, same as mReceiveMilliseconds.
    double previous = milliseconds.load(std::memory_order_relaxed);
    while(!milliseconds.compare_exchange_weak(previous, previous + elapsedMs, std::memory_order_relaxed)) {}
  }
};

inline Phase merge;
inline Phase mergeIdle;
inline Phase inject;
inline Phase injectIdle;
inline Phase filePathMaps;
inline Phase searchIndex;
inline Phase memberEdgeOrder;
inline Phase hierarchyCache;
inline Phase fullTextIndex;
inline Phase optimizeDatabase;

/** High-water mark of StorageProvider's queue: how far injection fell behind the workers. */
inline std::atomic<std::size_t> maxStorageQueueDepth{0};

inline void recordStorageQueueDepth(std::size_t depth) noexcept {
  std::size_t previous = maxStorageQueueDepth.load(std::memory_order_relaxed);
  while(depth > previous && !maxStorageQueueDepth.compare_exchange_weak(previous, depth, std::memory_order_relaxed)) {}
}

/** Times its scope into `phase` on destruction. */
class ScopedPhase final {
public:
  explicit ScopedPhase(Phase& phase) noexcept : mPhase(phase), mStart(std::chrono::steady_clock::now()) {}
  ScopedPhase(const ScopedPhase&) = delete;
  ScopedPhase& operator=(const ScopedPhase&) = delete;
  ScopedPhase(ScopedPhase&&) = delete;
  ScopedPhase& operator=(ScopedPhase&&) = delete;

  ~ScopedPhase() {
    mPhase.add(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - mStart).count());
  }

private:
  Phase& mPhase;
  std::chrono::steady_clock::time_point mStart;
};

/** Writes the INDEXER_TIMING line for everything above to stderr. */
void print();

}    // namespace indexing_stats
