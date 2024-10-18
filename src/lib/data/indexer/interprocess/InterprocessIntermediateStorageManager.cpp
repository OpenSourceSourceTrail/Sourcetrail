#include "InterprocessIntermediateStorageManager.h"

#include "IntermediateStorage.h"
#include "logging.h"
#include "SharedIntermediateStorage.h"

const char* InterprocessIntermediateStorageManager::sSharedMemoryNamePrefix = "iist_";

const char* InterprocessIntermediateStorageManager::sIntermediateStoragesKeyName = "intermediate_storages";

InterprocessIntermediateStorageManager::InterprocessIntermediateStorageManager(const std::string& instanceUuid,
                                                                               Id processId,
                                                                               bool isOwner)
    : BaseInterprocessDataManager(sSharedMemoryNamePrefix + std::to_string(processId) + "_" + instanceUuid,
                                  3 * 1048576 /* 3 MB */,
                                  instanceUuid,
                                  processId,
                                  isOwner) {}

InterprocessIntermediateStorageManager::~InterprocessIntermediateStorageManager() = default;

void InterprocessIntermediateStorageManager::pushIntermediateStorage(const std::shared_ptr<IntermediateStorage>& intermediateStorage) {
  const size_t requiredInsertsToShrink = 10;

  const size_t overestimationMultiplier = 2;
  const size_t requiredSize = (intermediateStorage->getByteSize(sizeof(SharedMemory::String)) + sizeof(SharedIntermediateStorage)) *
          overestimationMultiplier +
      1048576 /* 1 MB */;

  SharedMemory::ScopedAccess access(&mSharedMemory);

  const size_t freeMemory = access.getFreeMemorySize();
  if(freeMemory < requiredSize) {
    const size_t requiredGrowth = requiredSize - freeMemory;

    LOG_INFO(fmt::format("grow memory - est: {} size: {} free: {} alloc: {}",
                         requiredSize,
                         access.getMemorySize(),
                         access.getFreeMemorySize(),
                         requiredGrowth));

    access.growMemory(requiredGrowth);

    LOG_INFO("growing memory succeeded");

    mInsertsWithoutGrowth = 0;
  } else {
    mInsertsWithoutGrowth++;
  }

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIntermediateStorage>>(sIntermediateStoragesKeyName);
  if(!queue) {
    return;
  }

  queue->push_back(SharedIntermediateStorage(access.getAllocator()));
  SharedIntermediateStorage& storage = queue->back();

  storage.setStorageNodes(intermediateStorage->getStorageNodes());
  storage.setStorageFiles(intermediateStorage->getStorageFiles());
  storage.setStorageSymbols(intermediateStorage->getStorageSymbols());
  storage.setStorageEdges(intermediateStorage->getStorageEdges());
  storage.setStorageLocalSymbols(intermediateStorage->getStorageLocalSymbols());
  storage.setStorageSourceLocations(intermediateStorage->getStorageSourceLocations());
  storage.setStorageOccurrences(intermediateStorage->getStorageOccurrences());
  storage.setStorageComponentAccesses(intermediateStorage->getComponentAccesses());
  storage.setStorageErrors(intermediateStorage->getErrors());

  storage.setNextId(intermediateStorage->getNextId());

  if(mInsertsWithoutGrowth >= requiredInsertsToShrink) {
    mInsertsWithoutGrowth = 0;

    LOG_INFO("shrinking shared memory");
    access.shrinkToFitMemory();
    LOG_INFO(fmt::format("shrunk memory - size: {} free: {}", access.getMemorySize(), access.getFreeMemorySize()));
  }

  LOG_INFO(access.logString());
}

std::shared_ptr<IntermediateStorage> InterprocessIntermediateStorageManager::popIntermediateStorage() {
  SharedMemory::ScopedAccess access(&mSharedMemory);

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIntermediateStorage>>(sIntermediateStoragesKeyName);
  if(!queue || queue->empty()) {
    return nullptr;
  }

  SharedIntermediateStorage& sharedIntermediateStorage = queue->front();

  std::shared_ptr<IntermediateStorage> storage = std::make_shared<IntermediateStorage>();

  storage->setStorageNodes(sharedIntermediateStorage.getStorageNodes());
  storage->setStorageFiles(sharedIntermediateStorage.getStorageFiles());
  storage->setStorageSymbols(sharedIntermediateStorage.getStorageSymbols());
  storage->setStorageEdges(sharedIntermediateStorage.getStorageEdges());
  storage->setStorageLocalSymbols(sharedIntermediateStorage.getStorageLocalSymbols());
  storage->setStorageSourceLocations(sharedIntermediateStorage.getStorageSourceLocations());
  storage->setStorageOccurrences(sharedIntermediateStorage.getStorageOccurrences());
  storage->setComponentAccesses(sharedIntermediateStorage.getStorageComponentAccesses());
  storage->setErrors(sharedIntermediateStorage.getStorageErrors());

  storage->setNextId(sharedIntermediateStorage.getNextId());

  queue->pop_front();
  LOG_INFO(access.logString());

  return storage;
}

size_t InterprocessIntermediateStorageManager::getIntermediateStorageCount() {
  SharedMemory::ScopedAccess access(&mSharedMemory);

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIntermediateStorage>>(sIntermediateStoragesKeyName);
  if(!queue) {
    return 0;
  }

  return queue->size();
}
