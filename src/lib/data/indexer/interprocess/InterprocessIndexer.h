#pragma once
#include "InterprocessIndexerCommandManager.h"
#include "InterprocessIndexingStatusManager.h"
#include "InterprocessIntermediateStorageManager.h"

class InterprocessIndexer final {
public:
  InterprocessIndexer(const std::string& uuid, Id processId);

  void work();

private:
  InterprocessIndexerCommandManager mInterprocessIndexerCommandManager;
  InterprocessIndexingStatusManager mInterprocessIndexingStatusManager;
  InterprocessIntermediateStorageManager mInterprocessIntermediateStorageManager;

  const std::string mUuid;
  const Id mProcessId;
};