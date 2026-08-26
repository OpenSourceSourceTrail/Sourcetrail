#include "ClientFactory.h"

#include "HttpProject.h"
#include "MessageQueue.h"
#include "StorageCache.h"

namespace client {

ClientFactory::ClientFactory(EngineChannel* channel, std::weak_ptr<StorageAccess> storageAccess)
    : mChannel(channel), mStorageAccess(std::move(storageAccess)) {}

ClientFactory::~ClientFactory() = default;

std::shared_ptr<IProject> ClientFactory::createProject(std::shared_ptr<ProjectSettings> settings,
                                                       std::shared_ptr<StorageCache> storageCache,
                                                       std::string /*appUUID*/,
                                                       bool /*hasGUI*/) noexcept {
  if(storageCache) {
    storageCache->setSubject(mStorageAccess);
  }
  return std::make_shared<HttpProject>(mChannel, std::move(settings));
}

IMessageQueue::Ptr ClientFactory::createMessageQueue() noexcept {
  return std::make_shared<details::MessageQueue>();
}

lib::ISharedMemoryGarbageCollector::Ptr ClientFactory::createSharedMemoryGarbageCollector() noexcept {
  return nullptr;
}

}    // namespace client
