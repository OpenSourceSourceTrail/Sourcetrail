#pragma once

#include <memory>

#include "IndexerCommandType.h"

class IndexerCommand;
class IntermediateStorage;

class IndexerBase {
public:
  IndexerBase();
  virtual ~IndexerBase();

  [[nodiscard]] virtual IndexerCommandType getSupportedIndexerCommandType() const = 0;
  virtual std::shared_ptr<IntermediateStorage> index(std::shared_ptr<IndexerCommand> indexerCommand) = 0;
  virtual void interrupt() = 0;
};