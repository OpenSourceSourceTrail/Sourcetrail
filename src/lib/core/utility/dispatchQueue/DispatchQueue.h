#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include <condition_variable>

// Owns one worker thread that runs posted callbacks FIFO. Replaces the
// per-scheduler-id run loop that used to live in TaskScheduler.
class DispatchQueue final {
public:
  DispatchQueue();
  ~DispatchQueue();

  DispatchQueue(const DispatchQueue&) = delete;
  DispatchQueue& operator=(const DispatchQueue&) = delete;
  DispatchQueue(DispatchQueue&&) = delete;
  DispatchQueue& operator=(DispatchQueue&&) = delete;

  // Enqueue at the back of the queue.
  void post(std::function<void()> callback);

  // Enqueue at the front of the queue, so it runs before anything already queued.
  void postFront(std::function<void()> callback);

  // Enqueue at the back of the queue after delayMs elapse.
  void postDelayed(std::function<void()> callback, std::chrono::milliseconds delayMs);

  // Discards every entry not yet picked up by the worker. Whatever callback
  // is already running is unaffected and finishes normally.
  void clear();

  void stop();

private:
  struct Entry {
    std::function<void()> fn;
    std::chrono::steady_clock::time_point readyAt;
  };

  void run();

  std::thread mThread;
  std::deque<Entry> mEntries;
  mutable std::mutex mMutex;
  std::condition_variable mCondition;
  bool mStopped = false;
};
