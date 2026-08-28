#pragma once
#include "factory/IFactory.hpp"

class EngineChannel;
class StorageAccess;

namespace client {

/**
 * The factory a thin client builds its Application from.
 *
 * Where lib::Factory hands out a Project that owns a database, this hands out a HttpProject that
 * owns nothing and asks the engine, and points the application's StorageCache at the engine-backed
 * StorageAccess. That is the whole of the client/engine substitution: everything above IFactory is
 * identical in both processes.
 */
class ClientFactory final : public lib::IFactory {
public:
  /** `storageAccess` is installed as the StorageCache's subject on every project load. */
  ClientFactory(EngineChannel* channel, std::weak_ptr<StorageAccess> storageAccess);
  ~ClientFactory() override;

  ClientFactory(const ClientFactory&) = delete;
  ClientFactory(ClientFactory&&) = delete;
  ClientFactory& operator=(const ClientFactory&) = delete;
  ClientFactory& operator=(ClientFactory&&) = delete;

  std::shared_ptr<IProject> createProject(std::shared_ptr<ProjectSettings> settings,
                                          std::shared_ptr<StorageCache> storageCache,
                                          std::string appUUID,
                                          bool hasGUI) noexcept override;

  IMessageQueue::Ptr createMessageQueue() noexcept override;

  /**
   * Always null: shared memory exists to hand indexer processes their work, and a client never
   * starts one. The engine owns that machinery now.
   */
  lib::ISharedMemoryGarbageCollector::Ptr createSharedMemoryGarbageCollector() noexcept override;

private:
  EngineChannel* mChannel;
  std::weak_ptr<StorageAccess> mStorageAccess;
};

}    // namespace client
