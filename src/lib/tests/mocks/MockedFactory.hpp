#pragma once
#include <gmock/gmock.h>

#include "IFactory.hpp"
#include "Project.h"
#include "ProjectSettings.h"
#include "StorageCache.h"

namespace lib {

struct MockedFactory : IFactory {
  MOCK_METHOD(std::shared_ptr<IProject>,
              createProject,
              (std::shared_ptr<ProjectSettings> settings, std::shared_ptr<StorageCache> storageCache, std::string appUUID, bool hasGUI),
              (noexcept, override));
  MOCK_METHOD(IMessageQueue::Ptr, createMessageQueue, (), (noexcept, override));
  MOCK_METHOD(ISharedMemoryGarbageCollector::Ptr, createSharedMemoryGarbageCollector, (), (noexcept, override));
  MOCK_METHOD(scheduling::ITaskManager::Ptr, createTaskManager, (), (noexcept, override));
};

}    // namespace lib