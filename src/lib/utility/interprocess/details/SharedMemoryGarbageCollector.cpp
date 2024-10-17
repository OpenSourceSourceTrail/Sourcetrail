#include "SharedMemoryGarbageCollector.h"

#include <thread>

#include "logging.h"
#include "TimeStamp.h"
#include "utilityApp.h"

std::string SharedMemoryGarbageCollector::s_memoryNamePrefix = "grbg_cllctr_";
std::string SharedMemoryGarbageCollector::s_instancesKeyName = "running_instances";
std::string SharedMemoryGarbageCollector::s_timeStampsKeyName = "memory_to_timestamps";

const size_t SharedMemoryGarbageCollector::s_updateIntervalSeconds = 1;
const size_t SharedMemoryGarbageCollector::s_deleteThresholdSeconds = 10;

SharedMemoryGarbageCollector::SharedMemoryGarbageCollector(std::unique_ptr<SharedMemory> memory) noexcept
    : m_memory{std::move(memory)}, m_loopIsRunning(false) {}

SharedMemoryGarbageCollector::~SharedMemoryGarbageCollector() noexcept = default;

void SharedMemoryGarbageCollector::run(const std::string& uuid) noexcept {
  LOG_INFO("start shared memory garbage collection");

  m_uuid = uuid;

  m_thread = std::make_shared<std::thread>([this]() {
    m_loopIsRunning = true;

    while(m_loopIsRunning) {
      update();

      std::this_thread::sleep_for(std::chrono::seconds(s_updateIntervalSeconds));
    }
  });
}

void SharedMemoryGarbageCollector::stop() noexcept {
  LOG_INFO("stop shared memory garbage collection");

  m_loopIsRunning = false;

  m_thread->join();
  m_thread.reset();

  {
    std::lock_guard<std::mutex> lock(m_sharedMemoryNamesMutex);
    m_removedSharedMemoryNames.insert(m_sharedMemoryNames.begin(), m_sharedMemoryNames.end());
  }

  update();

  m_sharedMemoryNames.clear();
  m_removedSharedMemoryNames.clear();


  SharedMemory::ScopedAccess access(m_memory.get());

  SharedMemory::Map<SharedMemory::String, SharedMemory::String>* instances =
      access.accessValueWithAllocator<SharedMemory::Map<SharedMemory::String, SharedMemory::String>>(s_instancesKeyName);

  if(!instances) {
    return;
  }

  SharedMemory::String i(access.getAllocator());
  i = m_uuid.c_str();

  SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator itt = instances->find(i);
  if(itt != instances->end()) {
    instances->erase(itt);
  }

  bool otherRunningInstances = false;
  TimeStamp now = TimeStamp::now();
  for(SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator it = instances->begin(); it != instances->end();
      it++) {
    TimeStamp timestamp = TimeStamp(std::string(it->second.c_str()));
    if(now.deltaS(timestamp) <= s_deleteThresholdSeconds) {
      otherRunningInstances = true;
      LOG_INFO(fmt::format("currently running instance: {}", it->first.c_str()));
    }
  }

  if(!otherRunningInstances) {
    LOG_INFO(fmt::format("delete garbage collector memory: {}", getMemoryName()));
    SharedMemory::deleteSharedMemory(getMemoryName());
  }
}

void SharedMemoryGarbageCollector::registerSharedMemory(const std::string& sharedMemoryName) noexcept {
  {
    std::lock_guard<std::mutex> lock(m_sharedMemoryNamesMutex);
    m_sharedMemoryNames.insert(sharedMemoryName);
  }

  update();
}

void SharedMemoryGarbageCollector::unregisterSharedMemory(const std::string& sharedMemoryName) noexcept {
  {
    std::lock_guard<std::mutex> lock(m_sharedMemoryNamesMutex);
    size_t removedCount = m_sharedMemoryNames.erase(sharedMemoryName);

    if(removedCount > 0) {
      m_removedSharedMemoryNames.insert(sharedMemoryName);
    }
  }

  update();
}

std::string SharedMemoryGarbageCollector::getMemoryName() {
  return s_memoryNamePrefix + (utility::getApplicationArchitectureType() == APPLICATION_ARCHITECTURE_X86_32 ? "32" : "64");
}

void SharedMemoryGarbageCollector::update() {
  std::lock_guard<std::mutex> lock(m_sharedMemoryNamesMutex);

  SharedMemory::ScopedAccess access(m_memory.get());

  if(access.getFreeMemorySize() * 2 < access.getMemorySize()) {
    access.growMemory(access.getMemorySize());
    LOG_INFO(access.logString());
  }

  SharedMemory::String t(access.getAllocator());
  t = TimeStamp::now().toString().c_str();

  // update instances
  {
    SharedMemory::Map<SharedMemory::String, SharedMemory::String>* instances =
        access.accessValueWithAllocator<SharedMemory::Map<SharedMemory::String, SharedMemory::String>>(s_instancesKeyName);

    if(!instances) {
      return;
    }

    SharedMemory::String i(access.getAllocator());
    i = m_uuid.c_str();

    SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator it = instances->find(i);
    if(it != instances->end()) {
      it->second = t;
    } else {
      instances->insert(std::pair<SharedMemory::String, SharedMemory::String>(i, t));
    }
  }

  // update shared memories
  {
    SharedMemory::Map<SharedMemory::String, SharedMemory::String>* timeStamps =
        access.accessValueWithAllocator<SharedMemory::Map<SharedMemory::String, SharedMemory::String>>(s_timeStampsKeyName);

    if(!timeStamps) {
      return;
    }

    // remove deleted shared memories
    for(const std::string& name : m_removedSharedMemoryNames) {
      SharedMemory::String n(access.getAllocator());
      n = name.c_str();

      SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator it = timeStamps->find(n);
      if(it != timeStamps->end()) {
        timeStamps->erase(it);
      }
    }
    m_removedSharedMemoryNames.clear();

    // add or update shared memories
    for(const std::string& name : m_sharedMemoryNames) {
      SharedMemory::String n(access.getAllocator());
      n = name.c_str();

      SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator it = timeStamps->find(n);
      if(it != timeStamps->end()) {
        it->second = t;
      } else {
        timeStamps->insert(std::pair<SharedMemory::String, SharedMemory::String>(n, t));
      }
    }

    // delete old shared memories
    TimeStamp now = TimeStamp::now();
    for(SharedMemory::Map<SharedMemory::String, SharedMemory::String>::iterator it = timeStamps->begin(); it != timeStamps->end();) {
      TimeStamp timestamp = TimeStamp(std::string(it->second.c_str()));
      if(now.deltaS(timestamp) > s_deleteThresholdSeconds) {
        LOG_INFO(fmt::format("collect garbage: {}", it->first.c_str()));
        SharedMemory::deleteSharedMemory(it->first.c_str());
        timeStamps->erase(it++);
      } else {
        it++;
      }
    }
  }
}
