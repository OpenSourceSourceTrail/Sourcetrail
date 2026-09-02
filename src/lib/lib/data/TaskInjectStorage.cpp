#include "data/TaskInjectStorage.h"

#include <chrono>
#include <thread>
#include <utility>

#include "data/storage/Storage.h"
#include "data/storage/StorageProvider.h"
#include "indexing/domain/IndexingPhaseStats.h"

TaskInjectStorage::TaskInjectStorage(std::shared_ptr<StorageProvider> storageProvider, std::weak_ptr<Storage> target)
    : m_storageProvider(std::move(storageProvider)), m_target(std::move(target)) {}

void TaskInjectStorage::doEnter(std::shared_ptr<Blackboard> /*blackboard*/) {}

Task::TaskState TaskInjectStorage::doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(m_storageProvider->getStorageCount() > 0) {
    if(auto result = m_storageProvider->consumeLargestStorage()) {
      const auto source = std::move(result.value());
      // TODO(Hussein): What happen if lock failed but provider is consumed?!
      if(const auto target = m_target.lock()) {
        const indexing_stats::ScopedPhase timer(indexing_stats::inject);
        target->inject(source.get());
        return STATE_SUCCESS;
      }
    }
  }

  // Nothing queued. Same reasoning as TaskMergeStorages: back off only when there was no work.
  std::this_thread::sleep_for(std::chrono::milliseconds(indexing_stats::InjectStorageIdleDelayMs));
  indexing_stats::injectIdle.add(static_cast<double>(indexing_stats::InjectStorageIdleDelayMs));

  return STATE_FAILURE;
}

void TaskInjectStorage::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskInjectStorage::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskInjectStorage::handleMessage(MessageIndexingInterrupted* /*message*/) {
  m_storageProvider->clear();
}
