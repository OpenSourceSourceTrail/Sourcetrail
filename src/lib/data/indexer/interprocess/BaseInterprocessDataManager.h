#pragma once

#include <string>

#include "GlobalId.hpp"
#include "SharedMemory.h"

class BaseInterprocessDataManager {
public:
  BaseInterprocessDataManager(
      const std::string& sharedMemoryName, size_t initialSharedMemorySize, std::string instanceUuid, Id processId, bool isOwner);

  virtual ~BaseInterprocessDataManager() = default;

  [[nodiscard]] Id getProcessId() const;

protected:
  SharedMemory mSharedMemory;

  const std::string mInstanceUuid;
  const Id mProcessId;
};
