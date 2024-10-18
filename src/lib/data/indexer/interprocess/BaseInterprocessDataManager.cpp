#include "BaseInterprocessDataManager.h"

#include <utility>

BaseInterprocessDataManager::BaseInterprocessDataManager(
    const std::string& sharedMemoryName, size_t initialSharedMemorySize, std::string instanceUuid, Id processId, bool isOwner)
    : mSharedMemory(sharedMemoryName, initialSharedMemorySize, isOwner ? SharedMemory::CREATE_AND_DELETE : SharedMemory::OPEN_ONLY)
    , mInstanceUuid(std::move(instanceUuid))
    , mProcessId(processId) {}

Id BaseInterprocessDataManager::getProcessId() const {
  return mProcessId;
}
