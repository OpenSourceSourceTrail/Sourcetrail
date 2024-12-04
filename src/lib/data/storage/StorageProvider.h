#pragma once
#include <list>
#include <memory>
#include <mutex>

#include "IntermediateStorage.h"

class StorageProvider {
public:
  int getStorageCount() const;

  void clear();

  void insert(std::shared_ptr<IntermediateStorage> storage);

  // returns empty shared_ptr if no storages available
  std::shared_ptr<IntermediateStorage> consumeSecondLargestStorage();

  // returns empty shared_ptr if no storages available
  std::shared_ptr<IntermediateStorage> consumeLargestStorage();

  void logCurrentState() const;

private:
  std::list<std::shared_ptr<IntermediateStorage>> mStorages;    // larger storages are in front
  mutable std::mutex mStoragesMutex;
};