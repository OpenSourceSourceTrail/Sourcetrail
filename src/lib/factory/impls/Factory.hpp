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

  ISharedMemoryGarbageCollector::Ptr createSharedMemoryGarbageCollector() noexcept override;

  scheduling::ITaskManager::Ptr createTaskManager() noexcept override;
};

}    // namespace lib