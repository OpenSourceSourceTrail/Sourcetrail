#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include "ISharedMemoryGarbageCollector.hpp"
#include "SharedMemory.h"

class SharedMemoryGarbageCollector : public lib::ISharedMemoryGarbageCollector {
public:
  explicit SharedMemoryGarbageCollector(std::unique_ptr<SharedMemory> memory) noexcept;
  ~SharedMemoryGarbageCollector() noexcept override;

  void run(const std::string& uuid) noexcept override;
  void stop() noexcept override;

  void registerSharedMemory(const std::string& sharedMemoryName) noexcept override;
  void unregisterSharedMemory(const std::string& sharedMemoryName) noexcept override;

  static std::string getMemoryName();

private:
  void update();

  static std::string s_memoryNamePrefix;
  static std::string s_instancesKeyName;
  static std::string s_timeStampsKeyName;

  static const size_t s_updateIntervalSeconds;
  static const size_t s_deleteThresholdSeconds;

  std::unique_ptr<SharedMemory> m_memory;
  volatile bool m_loopIsRunning;
  std::shared_ptr<std::thread> m_thread;

  std::string m_uuid;

  std::mutex m_sharedMemoryNamesMutex;
  std::set<std::string> m_sharedMemoryNames;
  std::set<std::string> m_removedSharedMemoryNames;
};
