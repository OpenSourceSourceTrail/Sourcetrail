#pragma once

#include "IFactory.hpp"

namespace lib {

class Factory : public IFactory {
public:
  ~Factory() override;
  std::shared_ptr<IProject> createProject(std::shared_ptr<ProjectSettings> settings,
                                          StorageCache* storageCache,
                                          std::string appUUID,
                                          bool hasGUI) noexcept override;

  IMessageQueue::Ptr createMessageQueue() noexcept override;

#if !defined(SOURCETRAIL_WASM)
  ISharedMemoryGarbageCollector::Ptr createSharedMemoryGarbageCollector() noexcept override;
#endif

  scheduling::ITaskManager::Ptr createTaskManager() noexcept override;
};

}    // namespace lib