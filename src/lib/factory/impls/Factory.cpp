#include "Factory.hpp"

#include "details/SharedMemoryGarbageCollector.h"
#include "impls/TaskManager.hpp"
#include "logging.h"
#include "MessageQueue.h"
#include "Project.h"
#include "ProjectSettings.h"
#include "StorageCache.h"
#include "utilityApp.h"

namespace {
constexpr unsigned long long int operator""_kb(unsigned long long int size) noexcept {
  return size * 1024;
}
}    // namespace

namespace lib {

Factory::~Factory() = default;

std::shared_ptr<IProject> Factory::createProject(std::shared_ptr<ProjectSettings> settings,
                                                 std::shared_ptr<StorageCache> storageCache,
                                                 std::string appUUID,
                                                 bool hasGUI) noexcept {
  return std::make_shared<Project>(std::move(settings), std::move(storageCache), std::move(appUUID), hasGUI);
}

IMessageQueue::Ptr Factory::createMessageQueue() noexcept {
  return std::make_shared<details::MessageQueue>();
}

ISharedMemoryGarbageCollector::Ptr Factory::createSharedMemoryGarbageCollector() noexcept {
  SharedMemoryGarbageCollector::Ptr instance;
  try {
    auto memory = std::make_unique<SharedMemory>(
        SharedMemoryGarbageCollector::getMemoryName(), 64_kb, SharedMemory::OPEN_OR_CREATE);

    if(!memory->checkSharedMutex()) {
      LOG_ERROR("Shared memory mutex check failed. Shared memory garbage collection disabled.");
      return nullptr;
    }
    instance = std::make_shared<SharedMemoryGarbageCollector>(std::move(memory));
  } catch(boost::interprocess::interprocess_exception& e) {
    LOG_ERROR(fmt::format("boost exception thrown at shared memory garbage collector: {}", e.what()));
  }

  return instance;
}

scheduling::ITaskManager::Ptr Factory::createTaskManager() noexcept {
  return std::make_shared<scheduling::impls::TaskManager>();
}

}    // namespace lib