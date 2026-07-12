#include "DispatchQueue.h"

#include <algorithm>
#include <utility>

DispatchQueue::DispatchQueue() : mThread(&DispatchQueue::run, this) {}

DispatchQueue::~DispatchQueue() {
  stop();
}

void DispatchQueue::post(std::function<void()> callback) {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    mEntries.push_back(Entry{std::move(callback), std::chrono::steady_clock::now()});
  }
  mCondition.notify_one();
}

void DispatchQueue::postFront(std::function<void()> callback) {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    mEntries.push_front(Entry{std::move(callback), std::chrono::steady_clock::now()});
  }
  mCondition.notify_one();
}

void DispatchQueue::postDelayed(std::function<void()> callback, std::chrono::milliseconds delayMs) {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    mEntries.push_back(Entry{std::move(callback), std::chrono::steady_clock::now() + delayMs});
  }
  mCondition.notify_one();
}

void DispatchQueue::clear() {
  std::lock_guard<std::mutex> lock(mMutex);
  mEntries.clear();
}

void DispatchQueue::stop() {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if(mStopped) {
      return;
    }
    mStopped = true;
  }
  mCondition.notify_all();
  if(mThread.joinable()) {
    mThread.join();
  }
}

void DispatchQueue::run() {
  while(true) {
    std::function<void()> callback;

    {
      std::unique_lock<std::mutex> lock(mMutex);

      while(true) {
        if(mStopped) {
          return;
        }

        if(mEntries.empty()) {
          mCondition.wait(lock);
          continue;
        }

        auto readyIt = std::min_element(mEntries.begin(), mEntries.end(), [](const Entry& lhs, const Entry& rhs) {
          return lhs.readyAt < rhs.readyAt;
        });

        const auto now = std::chrono::steady_clock::now();
        if(readyIt->readyAt <= now) {
          callback = std::move(readyIt->fn);
          mEntries.erase(readyIt);
          break;
        }

        mCondition.wait_until(lock, readyIt->readyAt);
      }
    }

    if(callback) {
      callback();
    }
  }
}
