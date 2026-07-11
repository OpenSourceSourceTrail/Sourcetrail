#pragma once

#include "BaseInterprocessDataManager.h"
#include "SharedIndexerCommand.h"

class IndexerCommand;

class InterprocessIndexerCommandManager : public BaseInterprocessDataManager {
public:
  InterprocessIndexerCommandManager(const std::string& instanceUuid, Id processId, bool isOwner);
  ~InterprocessIndexerCommandManager() override;

  void pushIndexerCommands(const std::vector<std::shared_ptr<IndexerCommand>>& indexerCommands);
  std::shared_ptr<IndexerCommand> popIndexerCommand();

  void clearIndexerCommands();
  size_t indexerCommandCount();

private:
  static const char* sSharedMemoryNamePrefix;
  static const char* sIndexerCommandsKeyName;
};