#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include "SharedMemory.h"

struct ISharedMemoryGarbageCollector {
  static void setInstance(std::shared_ptr<ISharedMemoryGarbageCollector> instance);
  static bool createInstance();
  static std::shared_ptr<ISharedMemoryGarbageCollector> getInstance();
  static ISharedMemoryGarbageCollector* getInstanceRaw();

  ISharedMemoryGarbageCollector();
  virtual ~ISharedMemoryGarbageCollector();

  virtual void run(const std::string& uuid) = 0;
  virtual void stop() = 0;

  virtual void registerSharedMemory(const std::string& sharedMemoryName) = 0;
  virtual void unregisterSharedMemory(const std::string& sharedMemoryName) = 0;

private:
  static std::shared_ptr<ISharedMemoryGarbageCollector> sInstance;
};

class SharedMemoryGarbageCollector : public ISharedMemoryGarbageCollector {
public:
  static SharedMemoryGarbageCollector* createInstance();
  static SharedMemoryGarbageCollector* getInstance();

  SharedMemoryGarbageCollector();
  ~SharedMemoryGarbageCollector();

  void run(const std::string& uuid);
  void stop();

  void registerSharedMemory(const std::string& sharedMemoryName);
  void unregisterSharedMemory(const std::string& sharedMemoryName);

private:
  static std::string getMemoryName();
  void update();

  static std::string s_memoryNamePrefix;
  static std::string s_instancesKeyName;
  static std::string s_timeStampsKeyName;

  static const size_t s_updateIntervalSeconds;
  static const size_t s_deleteThresholdSeconds;

  SharedMemory m_memory;
  volatile bool m_loopIsRunning;
  std::shared_ptr<std::thread> m_thread;

  std::string m_uuid;

  std::mutex m_sharedMemoryNamesMutex;
  std::set<std::string> m_sharedMemoryNames;
  std::set<std::string> m_removedSharedMemoryNames;
};
