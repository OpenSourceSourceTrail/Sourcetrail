#include "StorageProvider.h"

#include "logging.h"

int StorageProvider::getStorageCount() const {
  const std::lock_guard<std::mutex> lock(mStoragesMutex);
  return static_cast<int>(mStorages.size());
}

void StorageProvider::clear() {
  const std::lock_guard<std::mutex> lock(mStoragesMutex);
  return mStorages.clear();
}

void StorageProvider::insert(std::shared_ptr<IntermediateStorage> storage) {
  const std::size_t storageSize = storage->getSourceLocationCount();
  std::list<std::shared_ptr<IntermediateStorage>>::iterator iterator;

  const std::lock_guard<std::mutex> lock(mStoragesMutex);
  for(iterator = mStorages.begin(); iterator != mStorages.end(); iterator++) {
    if((*iterator)->getSourceLocationCount() < storageSize) {
      break;
    }
  }
  mStorages.insert(iterator, std::move(storage));
}

std::shared_ptr<IntermediateStorage> StorageProvider::consumeSecondLargestStorage() {
  std::shared_ptr<IntermediateStorage> ret;
  {
    const std::lock_guard<std::mutex> lock(mStoragesMutex);
    if(mStorages.size() > 1) {
      auto iterator = mStorages.begin();
      iterator++;
      ret = *iterator;
      mStorages.erase(iterator);
    }
  }
  return ret;
}

std::shared_ptr<IntermediateStorage> StorageProvider::consumeLargestStorage() {
  std::shared_ptr<IntermediateStorage> ret;
  {
    const std::lock_guard<std::mutex> lock(mStoragesMutex);
    if(!mStorages.empty()) {
      ret = mStorages.front();
      mStorages.pop_front();
    }
  }

  return ret;
}

void StorageProvider::logCurrentState() const {
  std::string logString = "Storages waiting for injection:";
  {
    const std::lock_guard<std::mutex> lock(mStoragesMutex);
    for(const auto& storage : mStorages) {
      logString += " " + std::to_string(storage->getSourceLocationCount()) + ";";
    }
  }
  LOG_INFO(logString);
}
