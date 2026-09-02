#pragma once
#include <gmock/gmock.h>

#include "data/storage/StorageCache.h"
#include "factory/IFactory.hpp"
#include "project/data/Project.h"
#include "settings/ProjectSettings.h"

namespace lib {

struct MockedFactory : IFactory {
  MOCK_METHOD(std::shared_ptr<IProject>,
              createProject,
              (std::shared_ptr<ProjectSettings> settings, std::shared_ptr<StorageCache> storageCache, std::string appUUID, bool hasGUI),
              (noexcept, override));
  MOCK_METHOD(IMessageQueue::Ptr, createMessageQueue, (), (noexcept, override));
  MOCK_METHOD(ISharedMemoryGarbageCollector::Ptr, createSharedMemoryGarbageCollector, (), (noexcept, override));
};

}    // namespace lib