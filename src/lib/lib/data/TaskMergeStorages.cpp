#include "data/TaskMergeStorages.h"

#include <chrono>
#include <thread>
#include <utility>

#include "data/IndexingPhaseStats.h"
#include "data/storage/StorageProvider.h"

TaskMergeStorages::TaskMergeStorages(std::shared_ptr<StorageProvider> storageProvider)
    : m_storageProvider(std::move(storageProvider)) {}

void TaskMergeStorages::doEnter(std::shared_ptr<Blackboard> /*blackboard*/) {}

Task::TaskState TaskMergeStorages::doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(m_storageProvider->getStorageCount() > 2)    // largest storage won't be touched here
  {
    std::shared_ptr<IntermediateStorage> target;
    if(auto result = m_storageProvider->consumeSecondLargestStorage()) {
      target = result.value();
    }
    std::shared_ptr<IntermediateStorage> source;
    if(auto result = m_storageProvider->consumeSecondLargestStorage()) {
      source = result.value();
    }
    if(target && source) {
      const indexing_stats::ScopedPhase timer(indexing_stats::merge);
      target->inject(source.get());
      m_storageProvider->insert(target);
      return STATE_SUCCESS;
    } else {
      if(target) {
        m_storageProvider->insert(target);
      }
      if(source) {
        m_storageProvider->insert(source);
      }
    }
  }

  // Nothing to merge yet. The repeat decorator used to sleep between *all* iterations, which
  // throttled real merges to four a second; back off only on this path.
  std::this_thread::sleep_for(std::chrono::milliseconds(indexing_stats::MergeStorageIdleDelayMs));
  indexing_stats::mergeIdle.add(static_cast<double>(indexing_stats::MergeStorageIdleDelayMs));

  return STATE_FAILURE;
}

void TaskMergeStorages::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskMergeStorages::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}
