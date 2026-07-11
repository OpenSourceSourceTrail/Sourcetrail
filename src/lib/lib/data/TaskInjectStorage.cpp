#include "TaskInjectStorage.h"

#include <utility>

#include "Storage.h"
#include "StorageProvider.h"

TaskInjectStorage::TaskInjectStorage(std::shared_ptr<StorageProvider> storageProvider, std::weak_ptr<Storage> target)
    : m_storageProvider(std::move(storageProvider)), m_target(std::move(target)) {}

void TaskInjectStorage::doEnter(std::shared_ptr<Blackboard> /*blackboard*/) {}

Task::TaskState TaskInjectStorage::doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(m_storageProvider->getStorageCount() > 0) {
    if(auto result = m_storageProvider->consumeLargestStorage()) {
      const auto source = std::move(result.value());
      // TODO(Hussein): What happen if lock failed but provider is consumed?!
      if(const auto target = m_target.lock()) {
        target->inject(source.get());
        return STATE_SUCCESS;
      }
    }
  }

  return STATE_FAILURE;
}

void TaskInjectStorage::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskInjectStorage::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskInjectStorage::handleMessage(MessageIndexingInterrupted* /*message*/) {
  m_storageProvider->clear();
}
