#include "StorageProvider.h"

#include <range/v3/algorithm/find_if.hpp>

int StorageProvider::getStorageCount() const noexcept {
  const std::lock_guard lock(mStoragesMutex);
  return static_cast<int>(mStorages.size());
}

void StorageProvider::clear() {
  const std::lock_guard<std::mutex> lock(mStoragesMutex);
  mStorages.clear();
}

nonstd::expected<void, std::string> StorageProvider::insert(std::shared_ptr<IntermediateStorage> storage) noexcept {
  if(!storage) {
    return nonstd::make_unexpected("Storage is null");
  }
  const std::size_t storageSize = storage->getSourceLocationCount();

  const std::lock_guard lock(mStoragesMutex);
  const auto iterator = ranges::find_if(
      mStorages, [storageSize](const auto& currentStorage) { return currentStorage->getSourceLocationCount() < storageSize; });
  std::ignore = mStorages.insert(iterator, std::move(storage));
  return {};
}

nonstd::expected<std::shared_ptr<IntermediateStorage>, std::string> StorageProvider::consumeSecondLargestStorage() noexcept {
  {
    const std::lock_guard lock(mStoragesMutex);
    if(mStorages.size() > 1) {
      auto iterator = mStorages.begin();
      ++iterator;
      auto storage = *iterator;
      mStorages.erase(iterator);
      return storage;
    }
  }
  return nonstd::make_unexpected("No Storage found");
}

nonstd::expected<std::shared_ptr<IntermediateStorage>, std::string> StorageProvider::consumeLargestStorage() noexcept {
  {
    const std::lock_guard lock(mStoragesMutex);
    if(!mStorages.empty()) {
      auto ret = mStorages.front();
      mStorages.pop_front();
      return ret;
    }
  }
  return nonstd::make_unexpected("No Storage found");
}
