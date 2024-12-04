#include "InterprocessIndexerCommandManager.h"

#include "IndexerCommand.h"
#include "logging.h"

const char* InterprocessIndexerCommandManager::sSharedMemoryNamePrefix = "icmd_";

const char* InterprocessIndexerCommandManager::sIndexerCommandsKeyName = "indexer_commands";

constexpr auto OneMb = 1048576;

InterprocessIndexerCommandManager::InterprocessIndexerCommandManager(const std::string& instanceUuid, Id processId, bool isOwner)
    : BaseInterprocessDataManager(sSharedMemoryNamePrefix + instanceUuid, OneMb, instanceUuid, processId, isOwner) {}

InterprocessIndexerCommandManager::~InterprocessIndexerCommandManager() = default;

void InterprocessIndexerCommandManager::pushIndexerCommands(const std::vector<std::shared_ptr<IndexerCommand>>& indexerCommands) {
  size_t size = 0;
  {
    const size_t overestimationMultiplier = 2;
    for(const auto& command : indexerCommands) {
      size += command->getByteSize(sizeof(SharedMemory::String)) + sizeof(SharedIndexerCommand);
    }
    size *= overestimationMultiplier;
  }

  SharedMemory::ScopedAccess access(&mSharedMemory);
  while(access.getFreeMemorySize() < size) {
    const size_t currentSize = access.getMemorySize();
    LOG_INFO(fmt::format(
        "grow memory - est: {} size: {} free: {} alloc: {}", size, currentSize, access.getFreeMemorySize(), currentSize));

    access.growMemory(currentSize);

    LOG_INFO("growing memory succeeded");
  }

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIndexerCommand>>(sIndexerCommandsKeyName);
  if(queue == nullptr) {
    return;
  }

  for(const auto& command : indexerCommands) {
    queue->push_back(SharedIndexerCommand(access.getAllocator()));
    SharedIndexerCommand& sharedCommand = queue->back();
    sharedCommand.fromLocal(command.get());
  }

  LOG_INFO(access.logString());
}

std::shared_ptr<IndexerCommand> InterprocessIndexerCommandManager::popIndexerCommand() {
  SharedMemory::ScopedAccess access(&mSharedMemory);

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIndexerCommand>>(sIndexerCommandsKeyName);
  if((queue == nullptr) || queue->empty()) {
    return nullptr;
  }

  std::shared_ptr<IndexerCommand> command = SharedIndexerCommand::fromShared(queue->front());

  queue->pop_front();

  return command;
}

void InterprocessIndexerCommandManager::clearIndexerCommands() {
  SharedMemory::ScopedAccess access(&mSharedMemory);

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIndexerCommand>>(sIndexerCommandsKeyName);
  if(queue == nullptr) {
    return;
  }

  queue->clear();
}

size_t InterprocessIndexerCommandManager::indexerCommandCount() {
  SharedMemory::ScopedAccess access(&mSharedMemory);

  auto* queue = access.accessValueWithAllocator<SharedMemory::Queue<SharedIndexerCommand>>(sIndexerCommandsKeyName);
  if(queue == nullptr) {
    return 0;
  }

  return queue->size();
}
