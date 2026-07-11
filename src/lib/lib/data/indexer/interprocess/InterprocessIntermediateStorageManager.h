#pragma once

#include "BaseInterprocessDataManager.h"

class IntermediateStorage;

class InterprocessIntermediateStorageManager : public BaseInterprocessDataManager {
public:
  InterprocessIntermediateStorageManager(const std::string& instanceUuid, Id processId, bool isOwner);
  ~InterprocessIntermediateStorageManager() override;

  void pushIntermediateStorage(const std::shared_ptr<IntermediateStorage>& intermediateStorage);
  std::shared_ptr<IntermediateStorage> popIntermediateStorage();

  size_t getIntermediateStorageCount();

private:
  static const char* sSharedMemoryNamePrefix;
  static const char* sIntermediateStoragesKeyName;

  size_t mInsertsWithoutGrowth = 0;
};
