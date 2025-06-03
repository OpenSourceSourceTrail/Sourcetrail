#pragma once
#include <memory>

#if !defined(SOURCETRAIL_WASM)
#  include "ISharedMemoryGarbageCollector.hpp"
#endif
#include "ITaskManager.hpp"
#include "MessageQueue.h"
#include "ProjectSettings.h"

struct IProject;
class StorageCache;

namespace lib {

struct IFactory {
  virtual ~IFactory();
  virtual std::shared_ptr<IProject> createProject(std::shared_ptr<ProjectSettings> settings,
                                                  StorageCache* storageCache,
                                                  std::string appUUID,
                                                  bool hasGUI) noexcept = 0;

  virtual IMessageQueue::Ptr createMessageQueue() noexcept = 0;

#if !defined(SOURCETRAIL_WASM)
  virtual ISharedMemoryGarbageCollector::Ptr createSharedMemoryGarbageCollector() noexcept = 0;
#endif

  virtual scheduling::ITaskManager::Ptr createTaskManager() noexcept = 0;
};

}    // namespace lib